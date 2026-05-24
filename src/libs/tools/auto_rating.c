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
#include "common/collection.h"
#include "common/darktable.h"
#include "common/debug.h"
#include "common/image_cache.h"
#include "common/ratings.h"
#include "control/conf.h"
#include "control/control.h"
#include "control/jobs.h"
#include "control/signal.h"
#include "dtgtk/button.h"
#include "dtgtk/paint.h"
#include "gui/accelerators.h"
#include "gui/gtk.h"
#include "libs/lib.h"
#include "libs/lib_api.h"

DT_MODULE(1)

typedef struct dt_lib_auto_rating_t
{
  GtkWidget *button;
} dt_lib_auto_rating_t;

typedef struct dt_auto_rating_job_t
{
  GList *images;
  GList *rated; // images that were successfully rated (background thread writes these)
} dt_auto_rating_job_t;

typedef struct dt_auto_rating_idle_t
{
  GList *rated;
  guint processed;
  guint failed;
  guint total;
} dt_auto_rating_idle_t;

/* forward declarations */
static void _update_button_sensitivity(dt_lib_module_t *self);

/* --- job worker --- */

/*
  Low-level rating update that avoids GUI calls.
  Safe to use from background threads (writes to image cache + SQL only).
*/
static void _apply_rating_no_gui(const dt_imgid_t imgid, const int rating)
{
  dt_image_t *image = dt_image_cache_get(imgid, 'w');
  if(image)
  {
    image->flags = (image->flags & ~(DT_IMAGE_REJECTED | DT_VIEW_RATINGS_MASK))
                   | (DT_VIEW_RATINGS_MASK & rating);
    dt_image_cache_write_release_info(image, DT_IMAGE_CACHE_SAFE,
                                      "auto_rating_apply");
  }
}

/*
  Main-thread idle callback: emits signals and updates UI after
  all background scoring is complete.  Runs on the GTK main loop.
*/
static gboolean _auto_rating_idle_callback(gpointer data)
{
  dt_auto_rating_idle_t *idle = (dt_auto_rating_idle_t *)data;

  if(idle->rated)
  {
    DT_CONTROL_SIGNAL_RAISE(DT_SIGNAL_METADATA_CHANGED,
                            DT_METADATA_SIGNAL_NEW_VALUE);
  }

  if(idle->failed == 0)
    dt_control_log(ngettext("auto-rated %d image",
                            "auto-rated %d images", idle->processed),
                   idle->processed);
  else if(idle->processed == 0)
    dt_control_log(_("auto-rating failed for all images"));
  else
    dt_control_log(_("auto-rated %d of %d images (%d failed)"),
                   idle->processed, idle->total, idle->failed);

  g_list_free(idle->rated);
  g_free(idle);
  return G_SOURCE_REMOVE;
}

static int32_t _auto_rating_job_run(dt_job_t *job)
{
  dt_auto_rating_job_t *jd = (dt_auto_rating_job_t *)dt_control_job_get_params(job);
  if(!jd || !jd->images)
    return 1;

  dt_ai_rating_t *rating = dt_ai_rating_init();
  if(!rating)
  {
    if(darktable.ai_registry && darktable.ai_registry->ai_enabled)
    {
      dt_control_log(_("auto-rating: aesthetic rating model not found."
                       " download it via preferences > AI or Azure Artifacts"));
      dt_print(DT_DEBUG_AI,
               "[auto_rating] model 'rating-aesthetic-v1' not found."
               " download: az artifacts universal download"
               " --organization https://dev.azure.com/kamasylvia/"
               " --project Kamasylvia --scope project"
               " --feed Models --name rating-aesthetic-v1"
               " --version 1.0.0 --path ~/Downloads"
               " && mv ~/Downloads/rating-aesthetic-v1.dtmodel ~/.local/share/darktable/models/"
               " && darktable --ai-install-model ~/.local/share/darktable/models/rating-aesthetic-v1.dtmodel");
    }
    else
    {
      dt_control_log(_("auto-rating: AI features are disabled in preferences"));
    }
    return 1;
  }

  const guint total = g_list_length(jd->images);
  guint processed = 0;
  guint failed = 0;

  for(GList *l = jd->images; l; l = g_list_next(l))
  {
    const dt_imgid_t imgid = GPOINTER_TO_INT(l->data);
    const int stars = dt_ai_rating_score_image(rating, imgid);

    if(stars > 0)
    {
      _apply_rating_no_gui(imgid, stars);
      jd->rated = g_list_prepend(jd->rated, GINT_TO_POINTER(imgid));
      processed++;
    }
    else
    {
      failed++;
    }

    dt_control_job_set_progress(job, (double)(processed + failed) / (double)total);
  }

  dt_ai_rating_cleanup(rating);

  /* Schedule UI update on the main thread — signals must not be
     raised from background threads. */
  dt_auto_rating_idle_t *idle = g_new0(dt_auto_rating_idle_t, 1);
  idle->rated = g_list_reverse(jd->rated); // preserve original order
  idle->processed = processed;
  idle->failed = failed;
  idle->total = total;
  g_idle_add(_auto_rating_idle_callback, idle);

  return 0;
}

static void _auto_rating_job_cleanup(void *p)
{
  dt_auto_rating_job_t *jd = (dt_auto_rating_job_t *)p;
  if(jd)
  {
    g_list_free(jd->images);
    // jd->rated is transferred to the idle callback; do NOT free here
    g_free(jd);
  }
}

/* --- button click handler --- */

static void _button_clicked(GtkWidget *widget, dt_lib_module_t *self)
{
  GList *imgs = dt_act_on_get_images(FALSE, TRUE, FALSE);
  if(!imgs)
  {
    dt_control_log(_("no images selected for auto-rating"));
    return;
  }

  dt_auto_rating_job_t *jd = g_new0(dt_auto_rating_job_t, 1);
  jd->images = imgs;

  dt_job_t *job = dt_control_job_create(_auto_rating_job_run, _("auto-rating"));
  dt_control_job_set_params(job, jd, _auto_rating_job_cleanup);
  dt_control_job_add_progress(job, _("auto-rating images"), TRUE);
  dt_control_add_job(DT_JOB_QUEUE_USER_BG, job);
}

/* --- sensitivity update --- */

static void _update_button_sensitivity(dt_lib_module_t *self)
{
  dt_lib_auto_rating_t *d = (dt_lib_auto_rating_t *)self->data;
  if(!d || !d->button)
    return;

  gboolean sensitive = FALSE;
  if(darktable.ai_registry && darktable.ai_registry->ai_enabled)
  {
    dt_ai_model_t *model = dt_ai_models_get_by_id(darktable.ai_registry,
                                                   "rating-aesthetic-v1");
    if(model)
    {
      sensitive = (model->status == DT_AI_MODEL_DOWNLOADED);
      dt_ai_model_free(model);
    }
  }
  gtk_widget_set_sensitive(d->button, sensitive);
}

static void _on_ai_models_changed(gpointer instance, dt_lib_module_t *self)
{
  _update_button_sensitivity(self);
}

/* --- lib module API --- */

const char *name(dt_lib_module_t *self)
{
  return _("auto rating");
}

dt_view_type_flags_t views(dt_lib_module_t *self)
{
  return DT_VIEW_LIGHTTABLE;
}

uint32_t container(dt_lib_module_t *self)
{
  return DT_UI_CONTAINER_PANEL_CENTER_TOP_LEFT;
}

gboolean expandable(dt_lib_module_t *self)
{
  return FALSE;
}

int position(const dt_lib_module_t *self)
{
  return 2002;
}

void gui_init(dt_lib_module_t *self)
{
  dt_lib_auto_rating_t *d = g_malloc0(sizeof(dt_lib_auto_rating_t));
  self->data = (void *)d;

  self->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_valign(self->widget, GTK_ALIGN_CENTER);

  d->button = dtgtk_button_new(dtgtk_cairo_paint_wand, CPF_NONE, NULL);
  gtk_widget_set_tooltip_text(d->button,
    _("automatically rate selected images using AI aesthetic scoring"));
  g_signal_connect(G_OBJECT(d->button), "clicked",
                   G_CALLBACK(_button_clicked), self);

  gtk_box_pack_start(GTK_BOX(self->widget), d->button, FALSE, TRUE, 0);
  gtk_widget_show_all(self->widget);

  // Set initial sensitivity
  _update_button_sensitivity(self);

  // Listen for AI model changes (download/install/enable)
  DT_CONTROL_SIGNAL_HANDLE(DT_SIGNAL_AI_MODELS_CHANGED, _on_ai_models_changed);

  dt_action_t *ac = dt_action_define(&darktable.control->actions_global, NULL,
                                     N_("auto-rating"), d->button,
                                     &dt_action_def_button);
  dt_shortcut_register(ac, 0, 0, GDK_KEY_a, GDK_CONTROL_MASK);
}

void gui_cleanup(dt_lib_module_t *self)
{
  g_free(self->data);
  self->data = NULL;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
