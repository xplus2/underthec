#include "scene_internal.h"
#include "color.h"
#include "rng.h"
#include "xalloc.h"
#include <string.h>

#define FLAKE_COUNT 10
#define FLAKE_BAND_WIDTH 4
#define FLAKE_FALL_SPEED 2.0
#define FLAKE_TRICKLE_BASE 0.1
#define FLAKE_MAIN_BASE 0.5
#define FLAKE_CHAR_INTERVAL 3.0
#define FLAKE_BOTTOM_LINGER 3.0
#define FLAKE_SPEED_MULT_BASE 0.8
#define FLAKE_SPEED_MULT_RANGE 0.4
#define FLAKE_MAX_TRACKED_COLS 16
#define FEED_HEADING_SPEEDUP 1.5

static const char flake_chars[] = ",'`_-";

static char flake_pick_char(void) {
  return flake_chars[rng_int((int)sizeof(flake_chars) - 1)];
}

static void spawn_flake(struct scene *sc, int x) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_FLAKE;
  e->x = x;
  e->y = 0.0;
  e->z = Z_FLAKE;
  e->feed_speed_mult = FLAKE_SPEED_MULT_BASE + rng_double(FLAKE_SPEED_MULT_RANGE);
  e->default_attr = color_from_name("yellow");
  char *row = xmalloc(2);
  row[0] = flake_pick_char();
  row[1] = '\0';
  entity_set_owned_single_row(e, row, 0.0);
}

void feed_trigger(struct scene *sc, int w, int h) {
  (void)h;
  bool have_fish = false;
  double lowest_fish_y = 0.0;
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FISH) continue;
    if (!have_fish || e->y > lowest_fish_y) {
      lowest_fish_y = e->y;
      have_fish = true;
    }
  }
  if (have_fish) for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FLAKE) continue;
    if (e->y <= lowest_fish_y) return;
  }

  int max_band = w - FLAKE_BAND_WIDTH;
  if (max_band < 1) max_band = 1;
  int band_x0 = rng_int(max_band);
  for (int i = 0; i < FLAKE_COUNT; i++) spawn_flake(sc, band_x0 + rng_int(FLAKE_BAND_WIDTH));
  sc->feed_alerted = false;
}

static void feed_tick_flakes(struct scene *sc, int floor_row) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FLAKE) continue;
    if (e->die_after >= 0.0) continue;

    double old_y = e->y;
    double vy;
    if (old_y < WATER_SURFACE_ROW) vy = FLAKE_FALL_SPEED;
    else if (old_y < MAIN_REGION_TOP_ROW) vy = FLAKE_TRICKLE_BASE * e->feed_speed_mult;
    else vy = FLAKE_MAIN_BASE * e->feed_speed_mult;
    e->y = old_y + vy;
    if (e->y > floor_row) e->y = floor_row;

    bool crossed_surface = old_y < WATER_SURFACE_ROW && e->y >= WATER_SURFACE_ROW;
    if (crossed_surface) e->y = WATER_SURFACE_ROW;
    bool at_floor = e->y >= floor_row;
    if (at_floor) {
      e->owned_shape_rows[0][0][0] = '_';
      e->die_after = FLAKE_BOTTOM_LINGER;
    } else if (crossed_surface) {
      e->owned_shape_rows[0][0][0] = '_';
      e->frame_timer = 0.0;
    } else {
      e->frame_timer += 1.0;
      if (e->frame_timer >= FLAKE_CHAR_INTERVAL) {
        e->frame_timer -= FLAKE_CHAR_INTERVAL;
        e->owned_shape_rows[0][0][0] = flake_pick_char();
      }
    }
  }
}

static void feed_alert_fish(struct scene *sc) {
  if (sc->feed_alerted) return;
  int cols[FLAKE_MAX_TRACKED_COLS];
  int col_count = 0;
  bool any_in_main = false;
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FLAKE || e->die_after >= 0.0) continue;
    if (e->y >= MAIN_REGION_TOP_ROW) any_in_main = true;
    int col = (int)(e->x + 0.5);
    bool dup = false;
    for (int k = 0; k < col_count; k++) if (cols[k] == col) { dup = true; break; }
    if (!dup && col_count < FLAKE_MAX_TRACKED_COLS) cols[col_count++] = col;
  }
  if (!any_in_main || col_count == 0) return;

  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    if (fish->vy != 0.0 || fish->turn_state != TURN_NONE || fish->feed_heading) continue;
    int col = cols[rng_int(col_count)];
    int width = entity_width(fish);
    double front = fish->vx > 0 ? fish->x + width : fish->x;
    bool away = (fish->vx > 0 && (double)col < front) || (fish->vx < 0 && (double)col > front);
    fish->feed_target_col = col;
    fish->feed_heading = true;
    fish->feed_resume_vx = away ? -fish->vx : fish->vx;
    if (away) {
      fish->turn_state = TURN_SHRINK;
      fish->turn_step = 0;
      fish->turn_vx = -fish->vx * FEED_HEADING_SPEEDUP;
      fish->vx = 0.0;
    } else {
      fish->vx *= FEED_HEADING_SPEEDUP;
    }
  }
  sc->feed_alerted = true;
}

static void feed_advance_heading_fish(struct scene *sc) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    if (!fish->feed_heading || fish->feed_wait || fish->turn_state != TURN_NONE) continue;
    int width = entity_width(fish);
    double front = fish->vx > 0 ? fish->x + width : fish->x;
    bool reached = (fish->vx > 0 && front >= fish->feed_target_col) || (fish->vx < 0 && front <= fish->feed_target_col);
    if (reached) {
      fish->x = fish->vx > 0 ? fish->feed_target_col - width : fish->feed_target_col;
      fish->vx = 0.0;
      fish->feed_wait = true;
    }
  }
}

static bool fish_mouth_touch(struct entity *fish, const struct entity *flake) {
  ascii_rows rows = entity_shape(fish);
  int w = entity_width(fish);
  int h = entity_height(fish);
  if (rows == NULL || w <= 0 || h <= 0) return false;
  int front_cols[2];
  int found = 0;
  if (fish->vx >= 0.0) {
    for (int c = w - 1; c >= 0; c--) {
      if (found >= 2) break;
      for (int r = 0; r < h; r++) {
        const char *row = rows[r];
        if (row != NULL && c < (int)strlen(row) && row[c] != ' ') { front_cols[found++] = c; break; }
      }
    }
  } else {
    for (int c = 0; c < w; c++) {
      if (found >= 2) break;
      for (int r = 0; r < h; r++) {
        const char *row = rows[r];
        if (row != NULL && c < (int)strlen(row) && row[c] != ' ') { front_cols[found++] = c; break; }
      }
    }
  }
  if (found == 0) return false;
  int fx = (int)(fish->x + 0.5);
  int fy = (int)(fish->y + 0.5);
  int flake_row = (int)(flake->y + 0.5) - fy;
  if (flake_row < 0 || flake_row >= h) return false;
  int flake_col = (int)(flake->x + 0.5) - fx;
  for (int k = 0; k < found; k++) if (front_cols[k] == flake_col) return true;
  return false;
}

static void feed_check_touches(struct scene *sc) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH || !fish->feed_heading) continue;
    for (int j = 0; j < sc->entities.count; j++) {
      struct entity *flake = &sc->entities.items[j];
      if (flake->marked_dead || flake->type != ENT_FLAKE) continue;
      if (fish_mouth_touch(fish, flake)) {
        flake->marked_dead = true;
        fish->vx = fish->feed_resume_vx;
        fish->feed_heading = false;
        fish->feed_wait = false;
        break;
      }
    }
  }
}

static void feed_release_waiting_fish(struct scene *sc) {
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH || !fish->feed_heading) continue;
    bool any_above = false;
    for (int j = 0; j < sc->entities.count; j++) {
      const struct entity *flake = &sc->entities.items[j];
      if (flake->marked_dead || flake->type != ENT_FLAKE) continue;
      if (flake->y <= fish->y) { any_above = true; break; }
    }
    if (!any_above) {
      fish->vx = fish->feed_resume_vx;
      fish->feed_heading = false;
      fish->feed_wait = false;
    }
  }
}

void feed_tick(struct scene *sc, int w, int h) {
  (void)w;
  int floor_row = h - 1;
  feed_tick_flakes(sc, floor_row);
  feed_alert_fish(sc);
  feed_check_touches(sc);
  feed_advance_heading_fish(sc);
  feed_release_waiting_fish(sc);
}
