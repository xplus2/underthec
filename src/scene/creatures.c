#include "scene_internal.h"
#include "color.h"
#include "rng.h"

#include "art/bigfish.h"
#include "art/dolphins.h"
#include "art/ducks.h"
#include "art/fish.h"
#include "art/misc.h"
#include "art/monster.h"
#include "art/shark.h"
#include "art/ship.h"
#include "art/submarine.h"
#include "art/swan.h"
#include "art/swordfish.h"
#include "art/whale.h"

#include <stdlib.h>
#include <string.h>

static void randomize_fish_mask(struct entity *e, ascii_rows tmpl) {
  int rows = 0;
  while (tmpl[rows] != NULL) rows++;

  char **forced = malloc((size_t)(rows + 1) * sizeof(*forced));
  for (int i = 0; i < rows; i++) {
    size_t len = strlen(tmpl[i]);
    forced[i] = malloc(len + 1);
    for (size_t j = 0; j < len; j++) forced[i][j] = (tmpl[i][j] == '4') ? 'W' : tmpl[i][j];
    forced[i][len] = '\0';
  }
  forced[rows] = NULL;
  entity_randomize_mask(e, (ascii_rows)forced);
  for (int i = 0; i < rows; i++) free(forced[i]);
  free(forced);
}

static void spawn_fish_from_table(struct scene *sc, const struct sprite_pair *table, int pair_count, int w, int h) {
  int fish_num = rng_int(pair_count);
  bool odd = (fish_num % 2) != 0;
  double speed = rng_double(2.0) + 0.25;
  if (odd) speed = -speed;

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_FISH;
  e->z = rng_int(Z_FISH_RANGE) + Z_FISH_MIN;
  e->vx = speed;
  e->vy = 0;
  e->frames = &table[fish_num];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->physical = true;
  e->die_offscreen = true;
  e->death_action = DEATH_ADD_FISH;
  e->default_attr = (struct attr){COL_DEFAULT, false};

  if (table[fish_num].mask != NULL) randomize_fish_mask(e, table[fish_num].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  int max_height = 9;
  int min_height = h - height;
  e->y = rng_int(min_height - max_height) + max_height;
  e->x = odd ? (double)(w - 2) : (double)(1 - width);
}

void spawn_fish(struct scene *sc, int w, int h) {
  bool use_new = !sc->classic_mode && rng_int(12) > 8;
  if (use_new)
    spawn_fish_from_table(sc, fish_new, 8, w, h);
  else
    spawn_fish_from_table(sc, fish_old, 16, w, h);
}

static void spawn_big_fish_1(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double x, speed = 3.0;
  if (dir) {
    x = w - 1;
    speed = -3.0;
  } else {
    x = -34;
  }

  int max_height = 9, min_height = h - 15;
  int y = rng_int(min_height - max_height) + max_height;

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_BIGFISH;
  e->x = x;
  e->y = y;
  e->z = Z_BIGFISH;
  e->vx = speed;
  e->vy = 0;
  e->frames = &bigfish1[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("YELLOW");
  entity_randomize_mask(e, bigfish1[dir].mask);
}

static void spawn_big_fish_2(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double x, speed = 2.5;
  if (dir) {
    x = w - 1;
    speed = -2.5;
  } else {
    x = -33;
  }

  int max_height = 9, min_height = h - 14;
  int y = rng_int(min_height - max_height) + max_height;

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_BIGFISH;
  e->x = x;
  e->y = y;
  e->z = Z_BIGFISH;
  e->vx = speed;
  e->vy = 0;
  e->frames = &bigfish2[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("YELLOW");
  entity_randomize_mask(e, bigfish2[dir].mask);
}

static void spawn_big_fish(struct scene *sc, int w, int h) {
  bool use_2 = !sc->classic_mode && rng_int(3) > 1;
  if (use_2)
    spawn_big_fish_2(sc, w, h);
  else
    spawn_big_fish_1(sc, w, h);
}

static void spawn_shark(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double x, tx, speed = 2.0;
  int y = rng_int(h - 19) + 9;
  int ty = y + 7;

  if (dir) {
    speed = -2.0;
    x = w - 2;
    tx = x + 9;
  } else {
    x = -53;
    tx = -9;
  }

  struct entity *teeth = entity_spawn(&sc->entities);
  teeth->type = ENT_TEETH;
  teeth->x = tx;
  teeth->y = ty;
  teeth->z = Z_TEETH;
  teeth->vx = speed;
  teeth->vy = 0;
  teeth->frames = teeth_frame;
  teeth->frame_count = 1;
  teeth->physical = true;

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SHARK;
  e->x = x;
  e->y = y;
  e->z = Z_SHARK;
  e->vx = speed;
  e->vy = 0;
  e->frames = &shark[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_SHARK;
  e->default_attr = color_from_name("CYAN");
}

static void spawn_ship(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 1.0;
  if (dir) {
    speed = -1.0;
    x = w - 2;
  } else {
    x = -24;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SHIP;
  e->x = x;
  e->y = 0;
  e->z = Z_SHIP;
  e->vx = speed;
  e->vy = 0;
  e->frames = &ship[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("WHITE");
}

static void spawn_whale(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 1.0;
  if (dir) {
    speed = -1.0;
    x = w - 2;
  } else {
    x = -18;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_WHALE;
  e->x = x;
  e->y = 0;
  e->z = Z_WHALE;
  e->vx = speed;
  e->vy = 0;
  e->frames = whale[dir];
  e->frame_count = 12;
  e->frame_interval = 10.0; /* 1s/frame */
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("WHITE");
}

static void spawn_monster_new(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 2.0;
  if (dir) {
    speed = -2.0;
    x = w - 2;
  } else {
    x = -54;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_MONSTER;
  e->x = x;
  e->y = 2;
  e->z = Z_MONSTER;
  e->vx = speed;
  e->vy = 0;
  e->frames = monster_new[dir];
  e->frame_count = 2;
  e->frame_interval = 2.5; /* .25s/frame */
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("GREEN");
}

static void spawn_monster_old(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 2.0;
  if (dir) {
    speed = -2.0;
    x = w - 2;
  } else {
    x = -64;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_MONSTER;
  e->x = x;
  e->y = 2;
  e->z = Z_MONSTER;
  e->vx = speed;
  e->vy = 0;
  e->frames = monster_old[dir];
  e->frame_count = 4;
  e->frame_interval = 2.5;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("GREEN");
}

static void spawn_monster(struct scene *sc, int w, int h) {
  if (!sc->classic_mode)
    spawn_monster_new(sc, w, h);
  else
    spawn_monster_old(sc, w, h);
}

static void spawn_submarine(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 1.0;
  if (dir) {
    speed = -1.0;
    x = w - 2;
  } else {
    x = -40;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SUBMARINE;
  e->x = x;
  e->y = 6;
  e->z = Z_SUBMARINE;
  e->vx = speed;
  e->vy = 0;
  e->frames = submarine[dir];
  e->frame_count = 9;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("YELLOW");
}

static void spawn_swordfish(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double x, speed = 3.5;
  if (dir) {
    x = w - 1;
    speed = -3.5;
  } else {
    x = -33;
  }

  int max_height = 9, min_height = h - 14;
  int y = rng_int(min_height - max_height) + max_height;

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SWORDFISH;
  e->x = x;
  e->y = y;
  e->z = Z_SWORDFISH;
  e->vx = speed;
  e->vy = 0;
  e->frames = &swordfish[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("YELLOW");
  entity_randomize_mask(e, swordfish[dir].mask);
}

static void spawn_ducks(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 1.0;
  if (dir) {
    speed = -1.0;
    x = w - 2;
  } else {
    x = -30;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_DUCK;
  e->x = x;
  e->y = 5;
  e->z = Z_DUCK;
  e->vx = speed;
  e->vy = 0;
  e->frames = ducks[dir];
  e->frame_count = 3;
  e->frame_interval = 2.5; /* .25s/frame */
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("WHITE");
}

static void spawn_swan(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double x, speed = 1.0;
  if (dir) {
    speed = -1.0;
    x = w - 2;
  } else {
    x = -10;
  }

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SWAN;
  e->x = x;
  e->y = 1;
  e->z = Z_SWAN;
  e->vx = speed;
  e->vy = 0;
  e->frames = &swan[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("WHITE");
}

static void spawn_dolphins(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = 1.0, distance = 15.0, x;
  if (dir) {
    speed = -1.0;
    distance = -15.0;
    x = w - 2;
  } else {
    x = -13;
  }

  struct entity *e;
  e = entity_spawn(&sc->entities);
  e->type = ENT_DOLPHIN;
  e->x = x - distance * 2;
  e->y = 8;
  e->z = Z_DOLPHIN;
  e->vx = speed;
  e->vy = 0;
  e->age_ticks = 0;
  e->frames = dolphins[dir];
  e->frame_count = 2;
  e->frame_interval = 5.0;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_NONE;
  e->default_attr = color_from_name("blue");

  e = entity_spawn(&sc->entities);
  e->type = ENT_DOLPHIN;
  e->x = x - distance;
  e->y = 2;
  e->z = Z_DOLPHIN;
  e->vx = speed;
  e->vy = 0;
  e->age_ticks = 12;
  e->frames = dolphins[dir];
  e->frame_count = 2;
  e->frame_interval = 5.0;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_NONE;
  e->default_attr = color_from_name("BLUE");

  e = entity_spawn(&sc->entities);
  e->type = ENT_DOLPHIN;
  e->x = x;
  e->y = 5;
  e->z = Z_DOLPHIN;
  e->vx = speed;
  e->vy = 0;
  e->age_ticks = 24;
  e->frames = dolphins[dir];
  e->frame_count = 2;
  e->frame_interval = 5.0;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("CYAN");
}

void spawn_random_object(struct scene *sc, int w, int h) {
  switch (rng_int(11)) {
  case 0:
    spawn_ship(sc, w, h);
    break;
  case 1:
    spawn_whale(sc, w, h);
    break;
  case 2:
    spawn_monster(sc, w, h);
    break;
  case 3:
    spawn_big_fish(sc, w, h);
    break;
  case 4:
    spawn_shark(sc, w, h);
    break;
  case 5:
    spawn_submarine(sc, w, h);
    break;
  case 6:
    spawn_swordfish(sc, w, h);
    break;
  case 7:
    spawn_ducks(sc, w, h);
    break;
  case 8:
    spawn_dolphins(sc, w, h);
    break;
  case 9:
    spawn_swan(sc, w, h);
    break;
  default:
    spawn_fishhook(sc, w, h);
    break;
  }
}
