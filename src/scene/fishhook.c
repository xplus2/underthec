#include "scene_internal.h"
#include "color.h"
#include "rng.h"
#include "xalloc.h"

#include "art/fishhook.h"

#include <stdlib.h>
#include <string.h>

static int fishhook_body_height(void) {
  int n = 0;
  while (fishhook_image[n] != NULL) n++;
  return n;
}

static void grow_fishhook_rope(struct entity *e, int hook_h) {
  char **rows = e->owned_shape_rows[0];
  int old_total = 0;
  while (rows[old_total] != NULL) old_total++;
  int old_depth = old_total - hook_h;
  rows = xrealloc(rows, (size_t)(old_total + 2) * sizeof(*rows));
  memmove(&rows[old_depth + 1], &rows[old_depth], (size_t)(hook_h + 1) * sizeof(*rows));
  char *seg = xmalloc(8);
  memcpy(seg, "      |", 8);
  rows[old_depth] = seg;
  e->owned_shape_rows[0] = rows;
  e->owned_frame_table[0].shape = (ascii_rows)rows;
}

static void shrink_fishhook_rope(const struct entity *e, int hook_h) {
  char **rows = e->owned_shape_rows[0];
  int old_total = 0;
  while (rows[old_total] != NULL) old_total++;
  int old_depth = old_total - hook_h;
  free(rows[old_depth - 1]);
  memmove(&rows[old_depth - 1], &rows[old_depth], (size_t)(hook_h + 1) * sizeof(*rows));
}

static void update_fishhook_shape(struct entity *e, int depth) {
  if (depth < 0) depth = 0;
  int hook_h = fishhook_body_height();

  if (e->owned_shape_rows == NULL) {
    int total = depth + hook_h;
    char **rows = xmalloc((size_t)(total + 1) * sizeof(*rows));
    for (int i = 0; i < depth; i++) {
      rows[i] = xmalloc(8);
      memcpy(rows[i], "      |", 8);
    }
    for (int i = 0; i < hook_h; i++) {
      size_t len = strlen(fishhook_image[i]);
      rows[depth + i] = xmalloc(len + 1);
      memcpy(rows[depth + i], fishhook_image[i], len + 1);
    }
    rows[total] = NULL;
    char ***frame_list = xmalloc(1 * sizeof(*frame_list));
    frame_list[0] = rows;
    entity_set_owned_shape_frames(e, frame_list, 1, 0.0);
    entity_shape_changed(e);
    return;
  }

  char **rows = e->owned_shape_rows[0];
  int total = 0;
  while (rows[total] != NULL) total++;
  int cur_depth = total - hook_h;
  while (cur_depth < depth) {
    grow_fishhook_rope(e, hook_h);
    cur_depth++;
  }
  while (cur_depth > depth) {
    shrink_fishhook_rope(e, hook_h);
    cur_depth--;
  }
  if (total != depth + hook_h) entity_shape_changed(e);
}

void spawn_fishhook(struct scene *sc, int w, int h) {
  (void)h;
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_FISHHOOK;
  e->x = 10 + rng_int(w - 20);
  e->y = 0;
  e->z = Z_FISHHOOK;
  e->physical = true;
  e->trim_edges = true;
  e->sentinel = '?';
  e->splat_x = 0.0;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("GREEN");
  update_fishhook_shape(e, 0);
}

void fishhook_tick(struct scene *sc, int term_h) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *hook = &sc->entities.items[i];
    if (hook->marked_dead || hook->type != ENT_FISHHOOK) continue;
    if (hook->physical) {
      double resting = 0.75 * term_h - fishhook_body_height();
      if (hook->splat_x < resting) {
        hook->splat_x += 1.0;
        update_fishhook_shape(hook, (int)hook->splat_x);
      }
      struct sprite_pair barb_pair = { entity_shape(hook) + (int)hook->splat_x, NULL };
      struct entity barb = *hook;
      barb.frames = &barb_pair;
      barb.frame_count = 1;
      barb.frame_cur = 0;
      barb.wh_valid = false;
      barb.y = hook->y + hook->splat_x;
      for (int j = 0; j < sc->entities.count; j++) {
        struct entity *candidate = &sc->entities.items[j];
        if (candidate->marked_dead) continue;
        if (candidate->type != ENT_FISH && candidate->type != ENT_KAIJU) continue;
        if (!entity_glyph_overlap(&barb, candidate)) continue;
        hook->physical = false;
        candidate->physical = false;
        candidate->vx = 0;
        candidate->vy = -1;
        candidate->z = Z_HOOKED;
        break;
      }
    } else {
      hook->splat_x -= 1.0;
      if (hook->splat_x <= 0.0)
        hook->marked_dead = true;
      else
        update_fishhook_shape(hook, (int)hook->splat_x);
    }
  }
}
