#include "priv.h"

static const int periscope_hold_ticks[9] = {1, 4, 4, 9, 9, 9, 4, 4, 1};

static void tick_submarine(struct entity *e, int term_w) {
  double center = term_w / 2.0 - 20.0;
  bool crossing = (e->x < center && e->x + e->vx > center) || (e->x > center && e->x + e->vx < center);
  if (!crossing && e->x != center) {
    e->x += e->vx;
    return;
  }
  if (e->frame_cur < e->frame_count - 1) {
    if (e->frame_timer < periscope_hold_ticks[e->frame_cur]) {
      e->frame_timer += 1.0;
    } else {
      e->frame_timer = 0.0;
      e->frame_cur++;
      e->wh_valid = false;
    }
  } else {
    e->x += e->vx;
  }
}

static void tick_dolphin(struct entity *e) {
  int phase = e->age_ticks % 36;
  double dy;
  if (phase < 14)       dy = -0.5;
  else if (phase < 16)  dy = 0.0;
  else if (phase < 30)  dy = 0.5;
  else                  dy = 0.0;
  e->x += e->vx;
  e->y += dy;
  e->age_ticks++;
}

static void tick_bob(struct entity *e, int term_h) {
  int phase = e->age_ticks % 60;
  double dy;
  if (phase < 20)      dy = -0.15;
  else if (phase < 30) dy = 0.0;
  else if (phase < 50) dy = 0.15;
  else                 dy = 0.0;
  int height = entity_height(e);
  if (e->y < 6.0) dy = 0.15;
  else if (e->y + height > term_h - 2) dy = -0.15;
  e->x += e->vx;
  e->y += dy;
  e->age_ticks++;
}

static const double rower_speed_mult[5] = {2.0, 2.0, 2.0, 0.5, 0.5};

static void tick_rowers(struct entity *e) {
  int idx = e->frame_cur;
  double mult = (idx >= 0 && idx < 5) ? rower_speed_mult[idx] : 1.0;
  e->x += e->vx * mult;
}

static void advance_frame(struct entity *e) {
  if (e->frame_count <= 1 || e->frame_interval <= 0.0) return;
  e->frame_timer += 1.0;
  if (e->frame_timer >= e->frame_interval) {
    e->frame_timer -= e->frame_interval;
    e->frame_cur = (e->frame_cur + 1) % e->frame_count;
    e->wh_valid = false;
  }
}

void entity_tick_all(struct entity_list *list, int term_w, int term_h) {
  for (int i = 0; i < list->count; i++) {
    struct entity *e = &list->items[i];
    if (e->marked_dead) continue;
    e->prev_x = e->x;
    e->prev_y = e->y;
    e->has_prev = true;
    switch (e->type) {
      case ENT_SUBMARINE:      tick_submarine(e, term_w);   break;
      case ENT_LASER:
      case ENT_FISHHOOK:
      case ENT_KAIJU_TIMER:
      case ENT_RANDOM_OBJECT_TIMER:
      case ENT_FLAKE:
      case ENT_CASTLE_DOOR:                                 break;
      case ENT_DOLPHIN:        tick_dolphin(e);
                               advance_frame(e);            break;
      case ENT_JELLYFISH:
      case ENT_SEAHORSE:       tick_bob(e, term_h);
                               advance_frame(e);            break;
      case ENT_WATERLINE:
      case ENT_CASTLE:
      case ENT_FISH:
      case ENT_BUBBLE:
      case ENT_SPLAT:
      case ENT_TEETH:
      case ENT_SHARK:
      case ENT_SHIP:
      case ENT_WHALE:
      case ENT_MONSTER:
      case ENT_BIGFISH:
      case ENT_MESSAGE:
      case ENT_KAIJU:
      case ENT_SWORDFISH:
      case ENT_RUBBLE:
      case ENT_DUCK:
      case ENT_SWAN:
      case ENT_CRAB:           e->x += e->vx;
                               e->y += e->vy;
                               advance_frame(e);            break;
      case ENT_SEAWEED:        tick_seaweed_growth(e);
                               advance_frame(e);            break;
      case ENT_SEAWEED_DEBRIS: tick_seaweed_debris(e, term_h); break;
      case ENT_ROWERS:         tick_rowers(e);
                               advance_frame(e);            break;
    }
    if (e->type == ENT_CASTLE && e->frame_cur == e->frame_count - 1) e->frame_interval = 0.0;
    if (e->die_frame >= 0) {
      e->age_ticks++;
      if (e->age_ticks >= e->die_frame) {
        e->marked_dead = true;
        continue;
      }
    }
    if (e->die_after >= 0.0) {
      e->die_after -= 0.1;
      if (e->die_after <= 0.0) {
        e->marked_dead = true;
        continue;
      }
    }
    if (e->die_offscreen) {
      int w = entity_width(e);
      int h = entity_height(e);
      bool exiting_right = e->vx > 0.0 && e->x > term_w;
      bool exiting_left = e->vx < 0.0 && e->x + w < 0;
      bool exiting_down = e->vy > 0.0 && e->y > term_h;
      bool exiting_up = e->vy < 0.0 && e->y + h < 0;
      if (exiting_right || exiting_left || exiting_down || exiting_up) {
        e->marked_dead = true;
        continue;
      }
    }
  }
}
