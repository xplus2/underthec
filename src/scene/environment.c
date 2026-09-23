#include "scene_internal.h"
#include "color.h"
#include "rng.h"
#include "xalloc.h"

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

    char *tiled = xmalloc((size_t)unit_len * (size_t)repeat + 1);
    tiled[0] = '\0';
    for (int r = 0; r < repeat; r++) strcat(tiled, unit);

    struct entity *e = entity_spawn(&sc->entities);
    e->type = ENT_WATERLINE;
    e->x = 0;
    e->y = i + 5;
    e->z = depths[i];
    e->vx = speeds[i];
    e->splat_z = unit_len;
    e->physical = true;
    e->default_attr = color_from_name("cyan");
    entity_set_owned_single_row(e, tiled, 0.0);
  }
}

void add_castle(struct scene *sc, int w, int h) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_CASTLE;
  e->x = w - CASTLE_X_OFFSET;
  e->y = h - 13;
  e->z = Z_CASTLE;
  e->frames = &castle;
  e->frame_count = 1;
  e->default_attr = color_from_name("BLACK");
}

struct castle_reveal_ctx {
  int revealed_from_row;
  int row;
};

static void map_castle_reveal(const char *in, char *out, void *ctx) {
  struct castle_reveal_ctx *rv = ctx;
  if (rv->row < rv->revealed_from_row) out[0] = '\0';
  else strcpy(out, in);
  rv->row++;
}

static void map_identity_row(const char *in, char *out, void *ctx) {
  (void)ctx;
  strcpy(out, in);
}

void add_castle_building(struct scene *sc, int w, int h) {
  int rows = 0;
  while (castle_image[rows] != NULL) rows++;

  char ***frame_list = xmalloc((size_t)rows * sizeof(*frame_list));
  for (int step = 0; step < rows; step++) {
    struct castle_reveal_ctx ctx = {rows - 1 - step, 0};
    frame_list[step] = entity_build_transformed_rows(castle_image, map_castle_reveal, &ctx);
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_CASTLE;
  e->x = w - CASTLE_X_OFFSET;
  e->y = h - 13;
  e->z = Z_CASTLE;
  e->default_attr = color_from_name("BLACK");
  entity_set_owned_shape_frames(e, frame_list, rows, 8.0);
  e->owned_mask = entity_build_transformed_rows(castle_mask, map_identity_row, NULL);
}

void spawn_rubble(struct scene *sc, double castle_x, double castle_y, int castle_height) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_RUBBLE;
  e->frames = &rubble;
  e->frame_count = 1;
  e->trim_edges = true;
  e->z = Z_CASTLE;
  e->default_attr = color_from_name("BLACK");

  int rubble_height = entity_height(e);
  e->x = castle_x;
  e->y = castle_y + castle_height - rubble_height;
}

#define CASTLE_DOOR_ONE_IN 9000 /* ~15 min at 10 ticks/s */

static const int castle_door_hold_ticks[CASTLE_DOOR_STEPS] = {4, 4, 4, 2, 2, 2, 0, 2, 2, 4, 4, 4};

static double castle_door_hold(int step) {
  if (step == CASTLE_DOOR_OPEN_STEP) return rng_int(121) + 80.0; /* 8-20 s */
  return castle_door_hold_ticks[step];
}

static void spawn_castle_door(struct scene *sc, double x, double y) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_CASTLE_DOOR;
  e->x = x;
  e->y = y;
  e->z = Z_CASTLE_DOOR;
  e->frames = castle_door_frames;
  e->frame_count = CASTLE_DOOR_STEPS;
  e->frame_timer = castle_door_hold(0);
  e->default_attr = color_from_name("BLACK");
}

/* bubbles leaving top row keep rising */
static void release_door_bubbles(struct scene *sc, double door_x, double door_y) {
  const char *top = castle_door_frames[CASTLE_DOOR_OPEN_STEP - 1].shape[0];
  for (int col = 0; top[col] != '\0'; col++) {
    if (top[col] != 'o') continue;
    struct entity *b = spawn_bubble_at(sc, door_x + col, door_y - 1, Z_CASTLE_BUBBLE);
    b->frame_cur = 1;
  }
}

void castle_door_tick(struct scene *sc) {
  struct entity *castle_ent = entity_find_first(&sc->entities, ENT_CASTLE);
  bool castle_ready = castle_ent != NULL && sc->castle_hidden_by == 0 && castle_ent->frame_cur == castle_ent->frame_count - 1;
  struct entity *door = entity_find_first(&sc->entities, ENT_CASTLE_DOOR);

  if (door == NULL) {
    if (castle_ready && rng_int(CASTLE_DOOR_ONE_IN) == 0)
      spawn_castle_door(sc, castle_ent->x + CASTLE_DOOR_COL, castle_ent->y + CASTLE_DOOR_ROW);
    return;
  }
  if (!castle_ready) {
    door->marked_dead = true;
    return;
  }
  if (door->frame_timer > 1.0) {
    door->frame_timer -= 1.0;
    return;
  }

  int next = door->frame_cur + 1;
  if (next >= door->frame_count) {
    door->marked_dead = true;
    return;
  }
  door->frame_cur = next;
  door->frame_timer = castle_door_hold(next);
  if (next == CASTLE_DOOR_OPEN_STEP) release_door_bubbles(sc, door->x, door->y);
}

void add_seaweed(struct scene *sc, int w, int h) {
  int height = rng_int(4) + 3;
  char **rows0 = xmalloc((size_t)(height + 1) * sizeof(*rows0));
  char **rows1 = xmalloc((size_t)(height + 1) * sizeof(*rows1));
  for (int i = 1; i <= height; i++) {
    bool left = (i % 2) != 0;
    char *paren = xmalloc(2);
    memcpy(paren, "(", 2);
    char *gap = xmalloc(3);
    memcpy(gap, " )", 3);
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

  int castle_left = w - CASTLE_X_OFFSET;
  int castle_right = castle_left + CASTLE_WIDTH - 1;
  int left_lo = 1;
  int left_hi = castle_left - 1;
  int right_lo = castle_right + 1;
  int right_hi = w - 2;
  int left_span = left_hi >= left_lo ? left_hi - left_lo + 1 : 0;
  int right_span = right_hi >= right_lo ? right_hi - right_lo + 1 : 0;
  int total_span = left_span + right_span;
  int x;
  if (total_span <= 0) {
    x = rng_int(w - 2) + 1;
  } else {
    int pick = rng_int(total_span);
    x = pick < left_span ? left_lo + pick : right_lo + (pick - left_span);
  }
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

  char ***frame_list = xmalloc(2 * sizeof(*frame_list));
  frame_list[0] = rows0;
  frame_list[1] = rows1;
  entity_set_owned_shape_frames(e, frame_list, 2, anim_speed * 10.0);
}

void add_all_seaweed(struct scene *sc, int w, int h) {
  int count = w / 15;
  for (int i = 0; i < count; i++) add_seaweed(sc, w, h);
}

void add_all_fish(struct scene *sc, int w, int h) {
  int count;
  if (sc->aquatic.fish_count < 0) {
    int screen_size = (h - 9) * w;
    count = screen_size / 350;
  } else count = sc->aquatic.fish_count;
  for (int i = 0; i < count; i++) spawn_fish(sc, w, h);
}

static int message_row_count(const struct scene *sc) {
  int rows = 0;
  while (sc->message_rows[rows] != NULL) rows++;
  return rows;
}

static int message_block_width(const struct scene *sc, int rows) {
  int block_w = 0;
  for (int i = 0; i < rows; i++) {
    int len = entity_utf8_display_width(sc->message_rows[i]);
    if (len > block_w) block_w = len;
  }
  return block_w;
}

static int message_surface_y(int rows) {
  if (rows < WATER_SURFACE_ROW) return WATER_SURFACE_ROW - rows;
  return MESSAGE_TOP_ROW;
}

static struct entity *spawn_message_entity(struct scene *sc, double x, double y) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_MESSAGE;
  e->x = x;
  e->y = y;
  e->z = Z_MESSAGE;
  e->frames = &sc->message_frame;
  e->frame_count = 1;
  e->sentinel = ' ';
  e->default_attr = sc->message_attr;
  return e;
}

void add_message(struct scene *sc, int w, int h) {
  int rows = message_row_count(sc);
  if (rows == 0) return;
  int block_w = message_block_width(sc, rows);
  int y_mid = (h - rows) / 2;

  switch (sc->message_position) {
    case MSG_POS_EVENT:
      break; /* spawned via spawn_message_event, part of the random-object rotation */
    case MSG_POS_CENTER:
      spawn_message_entity(sc, (w - block_w) / 2, message_surface_y(rows));
      break;
    case MSG_POS_MARQUEE:
    case MSG_POS_SWIM: {
      bool swim = sc->message_position == MSG_POS_SWIM;
      double y = swim ? MESSAGE_TOP_ROW : y_mid;
      struct entity *e = spawn_message_entity(sc, w, y);
      if (swim) e->z = Z_MESSAGE_SURFACE;
      e->vx = -MESSAGE_SCROLL_SPEED;
      e->die_offscreen = true;
      e->death_action = DEATH_ADD_MESSAGE;
      break;
    }
    case MSG_POS_MIDDLE:
    default:
      spawn_message_entity(sc, (w - block_w) / 2, y_mid);
      break;
  }
}

void spawn_message_event(struct scene *sc, int w, int h) {
  (void)h;
  int rows = message_row_count(sc);
  if (rows == 0) return;

  struct entity *e = spawn_message_entity(sc, w, message_surface_y(rows));
  e->z = Z_MESSAGE_SURFACE;
  e->vx = -MESSAGE_SCROLL_SPEED;
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
}

void environment_tick(struct scene *sc) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *wl = &sc->entities.items[i];
    if (wl->marked_dead || wl->type != ENT_WATERLINE || wl->splat_z <= 0) continue;
    while (wl->x <= -wl->splat_z) wl->x += wl->splat_z;
    while (wl->x > 0) wl->x -= wl->splat_z;
  }
}
