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

#pragma once

#include "ai/backend.h"
#include "common/image.h"
#include <glib.h>

G_BEGIN_DECLS

/**
 * @brief Opaque aesthetic rating context.
 */
typedef struct dt_ai_rating_t dt_ai_rating_t;

/**
 * @brief Initialize the aesthetic rating module.
 *
 * Loads the active "rating" model from the AI registry if available.
 * Returns NULL when AI is disabled or no rating model is configured.
 *
 * @return Rating context, or NULL if unavailable.
 */
dt_ai_rating_t *dt_ai_rating_init(void);

/**
 * @brief Check if a rating model is available and ready.
 * @param rating The rating context.
 * @return TRUE if the model is loaded and ready.
 */
gboolean dt_ai_rating_available(const dt_ai_rating_t *rating);

/**
 * @brief Score a single image and return a 1-5 star rating.
 *
 * Loads the image's thumbnail, preprocesses it, runs inference,
 * and maps the aesthetic score to a star rating.
 *
 * @param rating The rating context.
 * @param imgid The image ID to score.
 * @return Rating 1-5, or 0 on error.
 */
int dt_ai_rating_score_image(dt_ai_rating_t *rating, const dt_imgid_t imgid);

/**
 * @brief Free the rating context and unload the model.
 * @param rating The rating context (NULL-safe).
 */
void dt_ai_rating_cleanup(dt_ai_rating_t *rating);

G_END_DECLS

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
