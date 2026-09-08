#include "scene_internal.h"
#include "color.h"
#include "rng.h"

#include "art/bigfish.h"
#include "art/crab.h"
#include "art/dolphins.h"
#include "art/ducks.h"
#include "art/fish.h"
#include "art/jellyfish.h"
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

int random_swim_y(int h, int sprite_height) {
  int max_height = 9;
  int min_height = h - sprite_height;
  return rng_int(min_height - max_height) + max_height;
}

void finish_creature_spawn(struct entity *e, enum entity_type type, int z, double vx, double vy, enum death_action death, struct attr attr) {
  e->type = type;
  e->z = z;
  e->vx = vx;
  e->vy = vy;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = death;
  e->default_attr = attr;
}

static void map_4_to_w(const char *in, char *out, void *ctx) {
  (void)ctx;
  size_t len = strlen(in);
  for (size_t j = 0; j < len; j++) out[j] = (in[j] == '4') ? 'W' : in[j];
  out[len] = '\0';
}

static void randomize_fish_mask(struct entity *e, ascii_rows tmpl) {
  char **forced = entity_build_transformed_rows(tmpl, map_4_to_w, NULL);
  entity_randomize_mask(e, (ascii_rows)forced);
  entity_free_owned_rows(forced);
}

static void spawn_fish_from_table(struct scene *sc, const struct sprite_pair *table, int pair_count, int w, int h) {
  int fish_num = rng_int(pair_count);
  bool odd = (fish_num % 2) != 0;
  double speed = rng_double(2.0) + 0.25;
  if (odd) speed = -speed;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &table[fish_num];
  e->frame_count = 1;
  if (table[fish_num].mask != NULL) randomize_fish_mask(e, table[fish_num].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = odd ? (double)(w - 2) : (double)(1 - width);
  finish_creature_spawn(e, ENT_FISH, rng_int(Z_FISH_RANGE) + Z_FISH_MIN, speed, 0, DEATH_ADD_FISH, (struct attr){COL_DEFAULT, false});
}

void spawn_fish(struct scene *sc, int w, int h) {
  bool use_new = !sc->classic_mode && rng_int(12) > 8;
  if (use_new) spawn_fish_from_table(sc, fish_new, 8, w, h);
  else spawn_fish_from_table(sc, fish_old, 16, w, h);
}

static void spawn_big_fish_1(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double speed = dir ? -3.0 : 3.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &bigfish1[dir];
  e->frame_count = 1;
  entity_randomize_mask(e, bigfish1[dir].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = dir ? (double)(w - 1) : (double)(1 - width);
  finish_creature_spawn(e, ENT_BIGFISH, Z_BIGFISH, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("YELLOW"));
}

static void spawn_big_fish_2(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double speed = dir ? -2.5 : 2.5;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &bigfish2[dir];
  e->frame_count = 1;
  entity_randomize_mask(e, bigfish2[dir].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = dir ? (double)(w - 1) : (double)(1 - width);
  finish_creature_spawn(e, ENT_BIGFISH, Z_BIGFISH, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("YELLOW"));
}

static void spawn_big_fish(struct scene *sc, int w, int h) {
  bool use_2 = !sc->classic_mode && rng_int(3) > 1;
  if (use_2) spawn_big_fish_2(sc, w, h);
  else spawn_big_fish_1(sc, w, h);
}

static void spawn_shark(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double x;
  double tx;
  double speed = 2.0;
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
  e->x = x;
  e->y = y;
  e->frames = &shark[dir];
  e->frame_count = 1;
  finish_creature_spawn(e, ENT_SHARK, Z_SHARK, speed, 0, DEATH_SHARK, color_from_name("CYAN"));
}

static void spawn_ship(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -1.0 : 1.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &ship[dir];
  e->frame_count = 1;
  e->y = 0;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));
  finish_creature_spawn(e, ENT_SHIP, Z_SHIP, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("WHITE"));
}

static void spawn_whale(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -1.0 : 1.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = whale[dir];
  e->frame_count = 12;
  e->frame_interval = 10.0;
  e->y = 0;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));
  finish_creature_spawn(e, ENT_WHALE, Z_WHALE, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("WHITE"));
}

static void spawn_monster_new(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -2.0 : 2.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = monster_new[dir];
  e->frame_count = 2;
  e->frame_interval = 2.5;
  e->y = 2;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));
  finish_creature_spawn(e, ENT_MONSTER, Z_MONSTER, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("GREEN"));
}

static void spawn_monster_old(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -2.0 : 2.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = monster_old[dir];
  e->frame_count = 4;
  e->frame_interval = 2.5;
  e->y = 2;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));

  finish_creature_spawn(e, ENT_MONSTER, Z_MONSTER, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("GREEN"));
}

static void spawn_monster(struct scene *sc, int w, int h) {
  if (!sc->classic_mode) spawn_monster_new(sc, w, h);
  else spawn_monster_old(sc, w, h);
}

static void spawn_submarine(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -1.0 : 1.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = submarine[dir];
  e->frame_count = 9;
  e->y = 6;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));
  finish_creature_spawn(e, ENT_SUBMARINE, Z_SUBMARINE, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("YELLOW"));
}

static void spawn_swordfish(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double speed = dir ? -3.5 : 3.5;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &swordfish[dir];
  e->frame_count = 1;
  entity_randomize_mask(e, swordfish[dir].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = dir ? (double)(w - 1) : (double)(1 - width);

  finish_creature_spawn(e, ENT_SWORDFISH, Z_SWORDFISH, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("YELLOW"));
}

static void spawn_ducks(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -1.0 : 1.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = ducks[dir];
  e->frame_count = 3;
  e->frame_interval = 2.5;
  e->y = 5;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));

  finish_creature_spawn(e, ENT_DUCK, Z_DUCK, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("WHITE"));
}

static void spawn_swan(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -1.0 : 1.0;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &swan[dir];
  e->frame_count = 1;
  e->y = 1;
  e->x = dir ? (double)(w - 2) : (double)(1 - entity_width(e));

  finish_creature_spawn(e, ENT_SWAN, Z_SWAN, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("WHITE"));
}

static void spawn_dolphins(struct scene *sc, int w, int h) {
  (void)h;
  int dir = rng_int(2);
  double speed = dir ? -1.0 : 1.0;
  double distance = dir ? -15.0 : 15.0;
  double x = dir ? (double)(w - 2) : -13.0;

  static const struct {
    double distance_mult;
    double y;
    int age_ticks;
    const char *color;
    enum death_action death;
  } pod[3] = {
      {2.0, 8.0, 0, "blue", DEATH_NONE},
      {1.0, 2.0, 12, "BLUE", DEATH_NONE},
      {0.0, 5.0, 24, "CYAN", DEATH_RANDOM_OBJECT},
  };

  for (int i = 0; i < 3; i++) {
    struct entity *e = entity_spawn(&sc->entities);
    e->x = x - distance * pod[i].distance_mult;
    e->y = pod[i].y;
    e->age_ticks = pod[i].age_ticks;
    e->frames = dolphins[dir];
    e->frame_count = 2;
    e->frame_interval = 5.0;
    finish_creature_spawn(e, ENT_DOLPHIN, Z_DOLPHIN, speed, 0, pod[i].death, color_from_name(pod[i].color));
  }
}

static void spawn_crab(struct scene *sc, int w, int h) {
  static const char *const colors[4] = {"red", "RED", "yellow", "YELLOW"};
  int dir = rng_int(2);
  double speed = rng_double(0.4) + 0.15;
  if (dir) speed = -speed;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = crab_frames;
  e->frame_count = 2;
  e->frame_interval = 3.0;

  int width = entity_width(e);
  int height = entity_height(e);
  e->y = h - height;
  e->x = dir ? (double)(w - 2) : (double)(1 - width);
  finish_creature_spawn(e, ENT_CRAB, Z_CRAB, speed, 0, DEATH_RANDOM_OBJECT, color_from_name(colors[rng_int(4)]));
}

void spawn_jellyfish(struct scene *sc, int w, int h) {
  if (!sc->aquatic.jellyfish) return;

  int dir = rng_int(2);
  double speed = rng_double(0.3) + 0.1;
  if (dir) speed = -speed;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = jellyfish_frames;
  e->frame_count = 2;
  e->frame_interval = 6.0;
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = dir ? (double)(w - 2) : (double)(1 - width);

  finish_creature_spawn(e, ENT_JELLYFISH, Z_JELLYFISH, speed, 0, DEATH_RANDOM_OBJECT, color_from_name("cyan"));
}

void spawn_random_object(struct scene *sc, int w, int h) {
  typedef void (*spawn_fn)(struct scene *, int, int);
  struct candidate {
    spawn_fn fn;
    bool enabled;
  };
  const struct candidate table[] = {
      {spawn_ship, sc->aquatic.ship},           {spawn_whale, sc->aquatic.whale},
      {spawn_monster, sc->aquatic.monster},     {spawn_big_fish, sc->aquatic.bigfish},
      {spawn_shark, sc->aquatic.shark},         {spawn_submarine, sc->aquatic.submarine},
      {spawn_swordfish, sc->aquatic.swordfish}, {spawn_ducks, sc->aquatic.ducks},
      {spawn_dolphins, sc->aquatic.dolphins},   {spawn_swan, sc->aquatic.swan},
      {spawn_fishhook, sc->aquatic.fishhook},   {spawn_crab, sc->aquatic.crab},
      {spawn_jellyfish, sc->aquatic.jellyfish},
  };
  const int count = (int)(sizeof(table) / sizeof(table[0]));
  spawn_fn enabled[13];
  int enabled_count = 0;
  for (int i = 0; i < count; i++) if (table[i].enabled) enabled[enabled_count++] = table[i].fn;
  if (enabled_count == 0) return;
  enabled[rng_int(enabled_count)](sc, w, h);
}
