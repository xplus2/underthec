#include "priv.h"
#include "rng.h"
#include "xalloc.h"

#include <string.h>

static char **seaweed_prepend_row(char **old_rows, int old_h, char *new_row) {
  char **out = xmalloc((size_t)(old_h + 2) * sizeof(*out));
  out[0] = new_row;
  for (int i = 0; i < old_h; i++) out[i + 1] = old_rows[i];
  out[old_h + 1] = NULL;
  free(old_rows);
  return out;
}

static void seaweed_grow_row(struct entity *e) {
  int h = entity_height(e);
  bool new_left = !e->seaweed_top_left;
  char *paren = xmalloc(2);
  memcpy(paren, "(", 2);
  char *gap = xmalloc(3);
  memcpy(gap, " )", 3);
  char *row0 = new_left ? paren : gap;
  char *row1 = new_left ? gap : paren;
  e->owned_shape_rows[0] = seaweed_prepend_row(e->owned_shape_rows[0], h, row0);
  e->owned_shape_rows[1] = seaweed_prepend_row(e->owned_shape_rows[1], h, row1);
  e->owned_frame_table[0].shape = (ascii_rows)e->owned_shape_rows[0];
  e->owned_frame_table[1].shape = (ascii_rows)e->owned_shape_rows[1];
  e->seaweed_top_left = new_left;
  e->seaweed_grown++;
  e->y -= 1.0;
  entity_shape_changed(e);
}

static bool seaweed_reached_limit(const struct entity *e) {
  if (e->seaweed_orig_height + e->seaweed_grown >= (e->seaweed_orig_height * 5 + 1) / 2) return true;
  return e->y <= 5.0;
}

static char *seaweed_row_mask(const char *shape_row, bool brown) {
  size_t len = strlen(shape_row);
  char *out = xmalloc(len + 1);
  for (size_t i = 0; i < len; i++) out[i] = (brown && shape_row[i] != ' ') ? 'y' : ' ';
  out[len] = '\0';
  return out;
}

static char **seaweed_mask_frame(char **shape_rows, int total_h, int grown) {
  char **out = xmalloc((size_t)(total_h + 1) * sizeof(*out));
  for (int i = 0; i < total_h; i++) out[i] = seaweed_row_mask(shape_rows[i], i < grown);
  out[total_h] = NULL;
  return out;
}

static void seaweed_cap(struct entity *e) {
  int total = entity_height(e);
  e->seaweed_full_collapse_in--;
  e->seaweed_full_collapse = e->seaweed_full_collapse_in <= 0;
  int brown_rows = e->seaweed_full_collapse ? total : e->seaweed_grown;
  char ***mask_frames = xmalloc(2 * sizeof(*mask_frames));
  mask_frames[0] = seaweed_mask_frame(e->owned_shape_rows[0], total, brown_rows);
  mask_frames[1] = seaweed_mask_frame(e->owned_shape_rows[1], total, brown_rows);
  entity_set_owned_shape_frame_masks(e, mask_frames);
  e->seaweed_capped = true;
  e->seaweed_split_timer = rng_double(180.0) + 90.0;
}

void tick_seaweed_growth(struct entity *e) {
  if (e->seaweed_capped) {
    if (e->seaweed_split_timer > 0.0) e->seaweed_split_timer -= 0.1;
    return;
  }
  e->seaweed_grow_timer -= 0.1;
  if (e->seaweed_grow_timer > 0.0) return;
  seaweed_grow_row(e);
  if (seaweed_reached_limit(e)) seaweed_cap(e);
  else e->seaweed_grow_timer = rng_double(180.0) + 90.0;
}

void tick_seaweed_debris(struct entity *e, int term_h) {
  if (e->seaweed_landed) return;
  e->y += e->vy;
  if (e->y < (double)(term_h - 1)) return;
  e->seaweed_landed = true;
  e->marked_dead = true;
}
