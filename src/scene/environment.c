#include "scene_internal.h"
#include "color.h"
#include "rng.h"

#include "art/castle.h"
#include "art/misc.h"

#include <stdlib.h>
#include <string.h>

void add_environment(struct scene *sc, int w, int h) {
  (void)h;
  static const ascii_rows segs[4] = {water_line_segment_0, water_line_segment_1, water_line_segment_2, water_line_segment_3};
  static const int depths[4] = {8, 6, 4, 2};
  static const double speeds[4] = {0.0, -0.05, -0.08, -0.12};

  for (int i = 0; i < 4; i++) {
    const char *unit = segs[i][0];
    int unit_len = (int)strlen(unit);
    int repeat = w / unit_len + 2;

    char *tiled = malloc((size_t)unit_len * (size_t)repeat + 1);
    tiled[0] = '\0';
    for (int r = 0; r < repeat; r++) strcat(tiled, unit);
    char **rows = malloc(2 * sizeof(*rows));
    rows[0] = tiled;
    rows[1] = NULL;

    char ***frame_list = malloc(1 * sizeof(*frame_list));
    frame_list[0] = rows;

    struct entity *e = entity_spawn(&sc->entities);
    e->type = ENT_WATERLINE;
    e->x = 0;
    e->y = i + 5;
    e->z = depths[i];
    e->vx = speeds[i];
    e->splat_z = unit_len; /* tile period, for wrap in environment_tick */
    e->physical = true;
    e->default_attr = color_from_name("cyan");
    entity_set_owned_shape_frames(e, frame_list, 1, 0.0);
  }
}

void add_castle(struct scene *sc, int w, int h) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_CASTLE;
  e->x = w - 32;
  e->y = h - 13;
  e->z = Z_CASTLE;
  e->frames = &castle;
  e->frame_count = 1;
  e->default_attr = color_from_name("BLACK");
}

void spawn_rubble(struct scene *sc, double castle_x, double castle_y, int castle_height) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_RUBBLE;
  e->frames = &rubble;
  e->frame_count = 1;
  e->trim_edges = true;
  e->z = Z_CASTLE;
  e->default_attr = color_from_name("yellow");

  int rubble_height = entity_height(e);
  e->x = castle_x;
  e->y = castle_y + castle_height - rubble_height;
}

void add_seaweed(struct scene *sc, int w, int h) {
  int height = rng_int(4) + 3;
  char **rows0 = malloc((size_t)(height + 1) * sizeof(*rows0));
  char **rows1 = malloc((size_t)(height + 1) * sizeof(*rows1));
  for (int i = 1; i <= height; i++) {
    bool left = (i % 2) != 0;
    char *paren = malloc(2);
    paren[0] = '(';
    paren[1] = '\0';
    char *gap = malloc(3);
    gap[0] = ' ';
    gap[1] = ')';
    gap[2] = '\0';
    if (left) {
      rows0[i - 1] = paren;
      rows1[i - 1] = gap;
    } else {
      rows1[i - 1] = paren;
      rows0[i - 1] = gap;
    }
  }
  rows0[height] = NULL;
  rows1[height] = NULL;

  int x = rng_int(w - 2) + 1;
  int y = h - height;
  double anim_speed = rng_double(0.05) + 0.25; /* seconds/frame */

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SEAWEED;
  e->x = x;
  e->y = y;
  e->z = Z_SEAWEED;
  e->default_attr = color_from_name("green");
  e->die_after = rng_double(4.0 * 60.0) + 8.0 * 60.0; /* 8-12 minutes */
  e->death_action = DEATH_ADD_SEAWEED;

  char ***frame_list = malloc(2 * sizeof(*frame_list));
  frame_list[0] = rows0;
  frame_list[1] = rows1;
  entity_set_owned_shape_frames(e, frame_list, 2, anim_speed * 10.0);
}

void add_all_seaweed(struct scene *sc, int w, int h) {
  int count = w / 15;
  for (int i = 0; i < count; i++) add_seaweed(sc, w, h);
}

void add_all_fish(struct scene *sc, int w, int h) {
  int screen_size = (h - 9) * w;
  int count = screen_size / 350;
  for (int i = 0; i < count; i++) spawn_fish(sc, w, h);
}

void add_message(struct scene *sc, int w, int h) {
  int rows = 0;
  while (sc->message_rows[rows] != NULL) rows++;
  if (rows == 0) return;
  int block_w = 0;
  for (int i = 0; i < rows; i++) {
    int len = (int)strlen(sc->message_rows[i]);
    if (len > block_w) block_w = len;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_MESSAGE;
  e->x = (w - block_w) / 2;
  e->y = (h - rows) / 2;
  e->z = Z_MESSAGE;
  e->frames = &sc->message_frame;
  e->frame_count = 1;
  e->sentinel = ' ';
  e->default_attr = color_from_name("blue");
}

void environment_tick(struct scene *sc) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *wl = &sc->entities.items[i];
    if (wl->marked_dead || wl->type != ENT_WATERLINE || wl->splat_z <= 0) continue;
    while (wl->x <= -wl->splat_z) wl->x += wl->splat_z;
    while (wl->x > 0) wl->x -= wl->splat_z;
  }
}
