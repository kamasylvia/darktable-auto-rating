/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common/ai/aesthetic_rating.h"
#include "common/ai_models.h"
#include "common/darktable.h"
#include "common/debug.h"
#include "common/mipmap_cache.h"
#include "control/conf.h"
#include "ai/backend.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DT_AI_RATING_TASK "rating"
#define DT_AI_RATING_MODEL_FILE "model.onnx"
#define DT_AI_RATING_DEFAULT_INPUT_SIZE 224

/*
  Mipmap buffers are CAIRO_FORMAT_RGB24: 4 bytes per pixel in native
  endian order (BGRx on little-endian, xRGB on big-endian).  We read
  the RGB channels by byte offset so the code is endian-safe.
*/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  #define R_OFF 2
  #define G_OFF 1
  #define B_OFF 0
#else
  #define R_OFF 1
  #define G_OFF 2
  #define B_OFF 3
#endif

struct dt_ai_rating_t
{
  dt_ai_environment_t *env;
  dt_ai_context_t *model;
  char *model_id;
  int input_size;       // model input H = W (square)
  gboolean normalize;   // TRUE = ImageNet mean/std, FALSE = [0,1]
  gboolean nima_output; // TRUE = 10-class probability distribution (NIMA)
  float threshold[7];   // user-defined thresholds for reject, 0-5 stars
  gboolean threshold_enabled[7];
};

/* --- image preprocessing --- */

/*
  Resize + normalize a source RGB image to the model's square input.
  out_size must equal ctx->input_size.  Uses bilinear interpolation.

  Source is 4-byte CAIRO_FORMAT_RGB24 (BGRx LE / xRGB BE).
  Output is interleaved float NHWC [1][out_size][out_size][3].
*/
static float *_preprocess_image(const uint8_t *src,
                                const int src_w,
                                const int src_h,
                                const int out_size,
                                const gboolean normalize)
{
  const size_t plane = (size_t)out_size * out_size;
  float *output = g_try_malloc0(3 * plane * sizeof(float));
  if(!output) return NULL;

  const float scale_x = (float)(src_w - 1) / (float)(out_size - 1);
  const float scale_y = (float)(src_h - 1) / (float)(out_size - 1);

  for(int y = 0; y < out_size; y++)
  {
    const float sy = y * scale_y;
    const int y0 = (int)sy;
    const int y1 = (y0 + 1 < src_h) ? y0 + 1 : y0;
    const float fy = sy - y0;

    for(int x = 0; x < out_size; x++)
    {
      const float sx = x * scale_x;
      const int x0 = (int)sx;
      const int x1 = (x0 + 1 < src_w) ? x0 + 1 : x0;
      const float fx = sx - x0;

      const uint8_t *p00 = src + ((y0 * src_w + x0) << 2);
      const uint8_t *p01 = src + ((y0 * src_w + x1) << 2);
      const uint8_t *p10 = src + ((y1 * src_w + x0) << 2);
      const uint8_t *p11 = src + ((y1 * src_w + x1) << 2);

      const float r = (p00[R_OFF] * (1.0f - fx) * (1.0f - fy)
                       + p01[R_OFF] * fx * (1.0f - fy)
                       + p10[R_OFF] * (1.0f - fx) * fy
                       + p11[R_OFF] * fx * fy);
      const float g = (p00[G_OFF] * (1.0f - fx) * (1.0f - fy)
                       + p01[G_OFF] * fx * (1.0f - fy)
                       + p10[G_OFF] * (1.0f - fx) * fy
                       + p11[G_OFF] * fx * fy);
      const float b = (p00[B_OFF] * (1.0f - fx) * (1.0f - fy)
                       + p01[B_OFF] * fx * (1.0f - fy)
                       + p10[B_OFF] * (1.0f - fx) * fy
                       + p11[B_OFF] * fx * fy);

      const size_t dst = (y * out_size + x) * 3;
      if(normalize)
      {
        // ImageNet normalization
        output[dst]     = (r - 123.675f) / 58.395f;
        output[dst + 1] = (g - 116.280f) / 57.120f;
        output[dst + 2] = (b - 103.530f) / 57.375f;
      }
      else
      {
        output[dst]     = r / 255.0f;
        output[dst + 1] = g / 255.0f;
        output[dst + 2] = b / 255.0f;
      }
    }
  }
  return output;
}

/* --- public API --- */

dt_ai_rating_t *dt_ai_rating_init(void)
{
  if(!darktable.ai_registry || !darktable.ai_registry->ai_enabled)
    return NULL;

  char *model_id = dt_ai_models_get_active_for_task(DT_AI_RATING_TASK);
  if(!model_id || !model_id[0])
  {
    g_free(model_id);
    return NULL;
  }

  dt_ai_environment_t *env = dt_ai_registry_get_env(darktable.ai_registry);
  if(!env)
  {
    g_free(model_id);
    return NULL;
  }

  dt_ai_context_t *ctx = dt_ai_load_model(env, model_id, DT_AI_RATING_MODEL_FILE,
                                          DT_AI_PROVIDER_CONFIGURED);
  if(!ctx)
  {
    dt_print(DT_DEBUG_AI, "[aesthetic_rating] failed to load model '%s'", model_id);
    g_free(model_id);
    return NULL;
  }

  // Query model info for optional attributes (input_size, normalize, nima_output)
  const dt_ai_model_info_t *info = dt_ai_get_model_info_by_id(env, model_id);
  int input_size = DT_AI_RATING_DEFAULT_INPUT_SIZE;
  gboolean normalize = TRUE;
  gboolean nima_output = FALSE;
  if(info)
  {
    input_size = dt_ai_model_attribute_int(info, "input_size", input_size);
    normalize  = dt_ai_model_attribute_bool(info, "normalize");
    nima_output = dt_ai_model_attribute_bool(info, "nima_output");
  }

  dt_ai_rating_t *rating = g_new0(dt_ai_rating_t, 1);
  rating->env = env;
  rating->model = ctx;
  rating->model_id = model_id;
  rating->input_size = input_size;
  rating->normalize = normalize;
  rating->nima_output = nima_output;

  // load user-defined rating thresholds
  static const char *threshold_keys[7] = {
    "plugins/ai/rating_threshold_reject",
    "plugins/ai/rating_threshold_0star",
    "plugins/ai/rating_threshold_1star",
    "plugins/ai/rating_threshold_2star",
    "plugins/ai/rating_threshold_3star",
    "plugins/ai/rating_threshold_4star",
    "plugins/ai/rating_threshold_5star"
  };
  static const char *enabled_keys[7] = {
    "plugins/ai/rating_threshold_reject_enabled",
    "plugins/ai/rating_threshold_0star_enabled",
    "plugins/ai/rating_threshold_1star_enabled",
    "plugins/ai/rating_threshold_2star_enabled",
    "plugins/ai/rating_threshold_3star_enabled",
    "plugins/ai/rating_threshold_4star_enabled",
    "plugins/ai/rating_threshold_5star_enabled"
  };
  static const float defaults[7] = {
    0.00f, 0.10f, 0.25f, 0.40f, 0.55f, 0.72f, 0.88f
  };
  static const gboolean default_enabled[7] = {
    FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE
  };

  for(int i = 0; i < 7; i++)
  {
    rating->threshold[i] = dt_conf_key_exists(threshold_keys[i])
                             ? dt_conf_get_float(threshold_keys[i])
                             : defaults[i];
    rating->threshold_enabled[i] = dt_conf_key_exists(enabled_keys[i])
                                     ? dt_conf_get_bool(enabled_keys[i])
                                     : default_enabled[i];
  }

  dt_print(DT_DEBUG_AI,
           "[aesthetic_rating] initialized model '%s' input_size=%d normalize=%d nima_output=%d",
           model_id, input_size, normalize, nima_output);
  return rating;
}

gboolean dt_ai_rating_available(const dt_ai_rating_t *rating)
{
  return rating != NULL && rating->model != NULL;
}

int dt_ai_rating_score_image(dt_ai_rating_t *rating, const dt_imgid_t imgid)
{
  if(!rating || !rating->model || !dt_is_valid_imgid(imgid))
    return 0;

  // Acquire a mipmap buffer for the image.
  // DT_MIPMAP_BEST_EFFORT automatically falls back to smaller/larger sizes.
  // Retry a few times to allow background thumbnail generation to catch up.
  dt_mipmap_buffer_t mip = {0};
  for(int attempt = 0; attempt < 3; attempt++)
  {
    dt_mipmap_cache_get(&mip, imgid, DT_MIPMAP_2, DT_MIPMAP_BEST_EFFORT, 'r');
    if(mip.buf) break;
    dt_mipmap_cache_release(&mip);
    g_usleep(50000); // 50ms
  }
  if(!mip.buf)
  {
    dt_print(DT_DEBUG_AI,
             "[aesthetic_rating] no mipmap available for image %d after retries", imgid);
    return 0;
  }

  const int src_w = mip.width;
  const int src_h = mip.height;

  // Preprocess: resize + normalize
  float *input = _preprocess_image(mip.buf, src_w, src_h,
                                   rating->input_size, rating->normalize);
  dt_mipmap_cache_release(&mip);

  if(!input)
  {
    dt_print(DT_DEBUG_AI,
             "[aesthetic_rating] preprocess failed for image %d", imgid);
    return 0;
  }

  // Prepare input tensor [N=1, H, W, C=3] (NHWC for TensorFlow models)
  const int64_t in_shape[4] = {1, rating->input_size, rating->input_size, 3};
  dt_ai_tensor_t in_tensor = {
    .data = input,
    .type = DT_AI_FLOAT,
    .shape = (int64_t *)in_shape,
    .ndim = 4,
  };

  float score = 0.0f;
  int ret = 0;

  if(rating->nima_output)
  {
    // NIMA: 10-class probability distribution for scores 1-10
    float probs[10] = {0.0f};
    const int64_t out_shape[2] = {1, 10};
    dt_ai_tensor_t out_tensor = {
      .data = probs,
      .type = DT_AI_FLOAT,
      .shape = (int64_t *)out_shape,
      .ndim = 2,
    };

    ret = dt_ai_run(rating->model, &in_tensor, 1, &out_tensor, 1);
    g_free(input);

    if(ret != 0)
    {
      dt_print(DT_DEBUG_AI,
               "[aesthetic_rating] inference failed for image %d (ret=%d)",
               imgid, ret);
      return 0;
    }

    // Weighted average of scores 1-10, then normalize to [0,1]
    float weighted = 0.0f;
    for(int i = 0; i < 10; i++)
      weighted += probs[i] * (float)(i + 1);

    score = (weighted - 1.0f) / 9.0f;
  }
  else
  {
    // Single float score directly in [0,1]
    const int64_t out_shape[1] = {1};
    dt_ai_tensor_t out_tensor = {
      .data = &score,
      .type = DT_AI_FLOAT,
      .shape = (int64_t *)out_shape,
      .ndim = 1,
    };

    ret = dt_ai_run(rating->model, &in_tensor, 1, &out_tensor, 1);
    g_free(input);

    if(ret != 0)
    {
      dt_print(DT_DEBUG_AI,
               "[aesthetic_rating] inference failed for image %d (ret=%d)",
               imgid, ret);
      return 0;
    }
  }

  // Clamp score to [0,1]
  if(score < 0.0f) score = 0.0f;
  if(score > 1.0f) score = 1.0f;

  // Map score to rating using user-defined thresholds.
  // Thresholds are ordered: [reject, 0-star, 1-star, 2-star, 3-star, 4-star, 5-star]
  // Rating values: reject=-1, 0-star=0, 1-star=1, ..., 5-star=5
  // Walk from highest rating downward; first enabled threshold that score meets wins.
  static const int rating_values[7] = { -1, 0, 1, 2, 3, 4, 5 };
  int stars = 1; // fallback
  gboolean found = FALSE;

  for(int i = 6; i >= 0; i--)
  {
    if(!rating->threshold_enabled[i]) continue;
    if(score >= rating->threshold[i])
    {
      stars = rating_values[i];
      found = TRUE;
      break;
    }
  }

  // If nothing matched (e.g. all thresholds above score), use the lowest enabled rating.
  if(!found)
  {
    for(int i = 0; i < 7; i++)
    {
      if(rating->threshold_enabled[i])
      {
        stars = rating_values[i];
        break;
      }
    }
  }

  dt_print(DT_DEBUG_AI,
           "[aesthetic_rating] image %d score=%.3f -> %d stars",
           imgid, score, stars);
  return stars;
}

void dt_ai_rating_cleanup(dt_ai_rating_t *rating)
{
  if(!rating) return;
  if(rating->model)
    dt_ai_unload_model(rating->model);
  g_free(rating->model_id);
  g_free(rating);
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
