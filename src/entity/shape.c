#include "priv.h"
#include "color.h"
#include "xalloc.h"

#include <string.h>

char **entity_build_transformed_rows(ascii_rows tmpl, row_transform_fn fn, void *ctx) {
  int rows = 0;
  while (tmpl[rows] != NULL) rows++;
  char **out = xmalloc((size_t)(rows + 1) * sizeof(*out));
  for (int i = 0; i < rows; i++) {
    size_t len = strlen(tmpl[i]);
    out[i] = xmalloc(len + 1);
    fn(tmpl[i], out[i], ctx);
  }
  out[rows] = NULL;
  return out;
}

static void randomize_row(const char *in, char *out, void *ctx) {
  (void)ctx;
  color_randomize_mask(in, out);
}

void entity_randomize_mask(struct entity *e, ascii_rows mask_template) {
  char **owned = entity_build_transformed_rows(mask_template, randomize_row, NULL);
  entity_clear_owned(e);
  e->owned_mask = owned;
}

void entity_set_owned_single_row(struct entity *e, char *row, double frame_interval_ticks) {
  char **rows = xmalloc(2 * sizeof(*rows));
  rows[0] = row;
  rows[1] = NULL;
  char ***frame_list = xmalloc(1 * sizeof(*frame_list));
  frame_list[0] = rows;
  entity_set_owned_shape_frames(e, frame_list, 1, frame_interval_ticks);
}

void entity_set_owned_shape_frames(struct entity *e, char ***rows, int frame_count, double frame_interval_ticks) {
  struct sprite_pair *table = xmalloc((size_t)frame_count * sizeof(*table));
  for (int i = 0; i < frame_count; i++) {
    table[i].shape = (ascii_rows)rows[i];
    table[i].mask = NULL;
  }
  e->owned_frame_table = table;
  e->owned_shape_rows = rows;
  e->frames = table;
  e->frame_count = frame_count;
  e->frame_cur = 0;
  e->frame_interval = frame_interval_ticks;
}

ascii_rows entity_shape(const struct entity *e) {
  if (e->frames == NULL || e->frame_count <= 0) return NULL;
  int idx = e->frame_cur;
  if (idx < 0 || idx >= e->frame_count) idx = 0;
  return e->frames[idx].shape;
}

void entity_set_owned_shape_frame_masks(struct entity *e, char ***mask_rows) {
  for (int i = 0; i < e->frame_count; i++) e->owned_frame_table[i].mask = (ascii_rows)mask_rows[i];
  e->owned_mask_frames = mask_rows;
}

ascii_rows entity_mask(const struct entity *e) {
  if (entity_shape(e) == NULL) return NULL;
  if (e->owned_mask != NULL) return (ascii_rows)e->owned_mask;
  int idx = e->frame_cur;
  if (idx < 0 || idx >= e->frame_count) idx = 0;
  return e->frames[idx].mask;
}

void entity_shape_changed(struct entity *e) {
  e->wh_valid = false;
}

static void compute_wh(struct entity *e) {
  ascii_rows rows = entity_shape(e);
  int w = 0;
  int h = 0;
  if (rows != NULL) {
    for (; rows[h] != NULL; h++) {
      int len = utf8_col_width(rows[h]);
      if (len > w) w = len;
    }
  }
  e->cached_w = w;
  e->cached_h = h;
  e->wh_valid = true;
}

int entity_height(struct entity *e) {
  if (!e->wh_valid) compute_wh(e);
  return e->cached_h;
}

int entity_width(struct entity *e) {
  if (!e->wh_valid) compute_wh(e);
  return e->cached_w;
}
