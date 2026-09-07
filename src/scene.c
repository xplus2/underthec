#include "scene.h"
#include "color.h"
#include "rng.h"

#include "art/bigfish.h"
#include "art/castle.h"
#include "art/dolphins.h"
#include "art/ducks.h"
#include "art/fish.h"
#include "art/fishhook.h"
#include "art/kaiju.h"
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

#define Z_SHARK 2
#define Z_TEETH 3
#define Z_FISH_MIN 3
#define Z_FISH_RANGE 17
#define Z_SEAWEED 21
#define Z_CASTLE 22
#define Z_MESSAGE 23
#define Z_SHIP 7
#define Z_WHALE 5
#define Z_MONSTER 5
#define Z_BIGFISH 2
#define Z_SUBMARINE 3
#define Z_SWORDFISH 2
#define Z_DUCK 3
#define Z_DOLPHIN 3
#define Z_SWAN 3
#define Z_FISHHOOK 7
#define Z_HOOKED 5

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

static void randomize_kaiju_mask(struct entity *e, ascii_rows tmpl) {
  static const char body_letters[4] = {'G','g','w','K'};
  static const char back_letters[5] = {'c','y','Y','R','r'};
  char body = body_letters[rng_int(4)];
  char back = back_letters[rng_int(5)];

  int rows = 0;
  while (tmpl[rows] != NULL) rows++;
  char **out = malloc((size_t)(rows + 1) * sizeof(*out));
  for (int i = 0; i < rows; i++) {
    size_t len = strlen(tmpl[i]);
    out[i] = malloc(len + 1);
    for (size_t j = 0; j < len; j++) {
      char ch = tmpl[i][j];
      if (ch == '2')
        out[i][j] = body;
      else if (ch == '3')
        out[i][j] = back;
      else
        out[i][j] = ch;
    }
    out[i][len] = '\0';
  }
  out[rows] = NULL;
  e->owned_mask = out;
}

static void spawn_splat(struct scene *sc, double tx, double ty, int tz) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SPLAT;
  e->x = tx - 4;
  e->y = ty - 2;
  e->z = tz - 2;
  e->frames = splat_frames;
  e->frame_count = 4;
  e->frame_interval = 2.5; /* .25s per frame */
  e->die_frame = 15;
  e->sentinel = ' ';
  e->default_attr = color_from_name("RED");
}

static void spawn_bubble(struct scene *sc, double fish_x, double fish_y, int fish_z, int fish_w, int fish_h, double fish_vx) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_BUBBLE;
  e->x = fish_x + (fish_vx > 0 ? fish_w : 0);
  e->y = fish_y + fish_h / 2;
  e->z = fish_z - 1;
  e->vx = 0;
  e->vy = -1;
  e->frames = bubble_frames;
  e->frame_count = 5;
  e->frame_interval = 1.0; /* .1s/frame */
  e->die_offscreen = true;
  e->physical = true;
  e->default_attr = color_from_name("CYAN");
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

static void spawn_fish(struct scene *sc, int w, int h) {
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

static int fishhook_body_height(void) {
  int n = 0;
  while (fishhook_image[n] != NULL) n++;
  return n;
}

static void update_fishhook_shape(struct entity *e, int depth) {
  if (depth < 0) depth = 0;

  int hook_h = fishhook_body_height();
  int total = depth + hook_h;

  char **rows = malloc((size_t)(total + 1) * sizeof(*rows));
  for (int i = 0; i < depth; i++) {
    rows[i] = malloc(8);
    memcpy(rows[i], "      |", 8);
  }
  for (int i = 0; i < hook_h; i++) {
    size_t len = strlen(fishhook_image[i]);
    rows[depth + i] = malloc(len + 1);
    memcpy(rows[depth + i], fishhook_image[i], len + 1);
  }
  rows[total] = NULL;
  entity_clear_owned(e);
  char ***frame_list = malloc(1 * sizeof(*frame_list));
  frame_list[0] = rows;
  entity_set_owned_shape_frames(e, frame_list, 1, 0.0);
}

static void spawn_fishhook(struct scene *sc, int w, int h) {
  (void)h;
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_FISHHOOK;
  e->x = 10 + rng_int(w - 20);
  e->y = 0;
  e->z = Z_FISHHOOK;
  e->physical = true;
  e->splat_x = 0.0;
  e->death_action = DEATH_RANDOM_OBJECT;
  e->default_attr = color_from_name("GREEN");
  update_fishhook_shape(e, 0);
}

static void spawn_random_object(struct scene *sc, int w, int h) {
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

static void add_environment(struct scene *sc, int w, int h) {
  (void)h;
  static const ascii_rows segs[4] = {water_line_segment_0, water_line_segment_1, water_line_segment_2, water_line_segment_3};
  static const int depths[4] = {8, 6, 4, 2};

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
    e->physical = true;
    e->default_attr = color_from_name("cyan");
    entity_set_owned_shape_frames(e, frame_list, 1, 0.0);
  }
}

static void add_castle(struct scene *sc, int w, int h) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_CASTLE;
  e->x = w - 32;
  e->y = h - 13;
  e->z = Z_CASTLE;
  e->frames = &castle;
  e->frame_count = 1;
  e->default_attr = color_from_name("BLACK");
}

static void spawn_rubble(struct scene *sc, double castle_x, double castle_y, int castle_height) {
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

static void spawn_kaiju(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double speed = rng_double(2.0) + 0.25;
  if (dir) speed = -speed;

  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_KAIJU;
  e->z = rng_int(Z_FISH_RANGE) + Z_FISH_MIN;
  e->vx = speed;
  e->vy = 0;
  e->frames = &kaiju[dir];
  e->frame_count = 1;
  e->trim_edges = true;
  e->sentinel = '?';
  e->die_offscreen = true;
  e->death_action = DEATH_ADD_KAIJU;
  e->default_attr = (struct attr){COL_DEFAULT, false};
  randomize_kaiju_mask(e, kaiju[dir].mask);

  int width = entity_width(e);
  int height = entity_height(e);
  int max_height = 9;
  int min_height = h - height;
  e->y = rng_int(min_height - max_height) + max_height;
  e->x = dir ? (double)(w - 2) : (double)(1 - width);
}

static bool find_kaiju_eye(const struct entity *kaiju, int *row_out, int *col_out) {
  ascii_rows rows = entity_shape(kaiju);
  if (rows == NULL) return false;
  for (int r = 0; rows[r] != NULL; r++) {
    int sum = 0, count = 0;
    for (int c = 0; rows[r][c] != '\0'; c++) if (rows[r][c] == '0') {
      sum += c;
      count++;
    }
    if (count > 0) {
      *row_out = r;
      *col_out = sum / count;
      return true;
    }
  }
  return false;
}

static void update_laser_shape(struct entity *e, int length) {
  if (length < 1) length = 1;
  entity_clear_owned(e);

  char *line = malloc((size_t)length + 1);
  memset(line, '=', (size_t)length);
  line[length] = '\0';

  char **rows = malloc(2 * sizeof(*rows));
  rows[0] = line;
  rows[1] = NULL;
  char ***frame_list = malloc(1 * sizeof(*frame_list));
  frame_list[0] = rows;
  entity_set_owned_shape_frames(e, frame_list, 1, 0.0);

  e->x = (e->vx < 0) ? e->splat_x - (length - 1) : e->splat_x;
}

static void spawn_laser(struct scene *sc, double eye_x, double eye_y, int z, int target_id, double dir_sign) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_LASER;
  e->y = eye_y;
  e->z = z;
  e->vx = dir_sign;
  e->splat_x = eye_x;
  e->splat_z = target_id;
  e->default_attr = color_from_name("RED");
  e->age_ticks = 1;
  update_laser_shape(e, 1);
}

static void add_seaweed(struct scene *sc, int w, int h) {
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

static void add_all_seaweed(struct scene *sc, int w, int h) {
  int count = w / 15;
  for (int i = 0; i < count; i++) add_seaweed(sc, w, h);
}

static void add_all_fish(struct scene *sc, int w, int h) {
  int screen_size = (h - 9) * w;
  int count = screen_size / 350;
  for (int i = 0; i < count; i++) spawn_fish(sc, w, h);
}

static void add_message(struct scene *sc, int w, int h) {
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

struct scene_ctx {
  struct scene *sc;
  int w, h;
};

static void on_death(const struct entity *dead, void *ctx) {
  struct scene_ctx *sctx = ctx;

  if (dead->spawn_splat) spawn_splat(sctx->sc, dead->splat_x, dead->splat_y, dead->splat_z);
  if (dead->type == ENT_KAIJU && dead->id == sctx->sc->castle_hidden_by) {
    add_castle(sctx->sc, sctx->w, sctx->h);
    sctx->sc->castle_hidden_by = 0;
  }
  switch (dead->death_action) {
  case DEATH_ADD_FISH:
    spawn_fish(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_ADD_SEAWEED:
    add_seaweed(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_ADD_KAIJU:
    spawn_kaiju(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_RANDOM_OBJECT:
  case DEATH_SHARK:
    spawn_random_object(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_NONE:
  default:
    break;
  }
}

void scene_init(struct scene *sc, bool classic_mode) {
  entity_list_init(&sc->entities);
  sc->classic_mode = classic_mode;
  sc->message_rows = NULL;
  sc->message_frame.shape = NULL;
  sc->message_frame.mask = NULL;
}

void scene_free(struct scene *sc) {
  entity_list_free(&sc->entities);
  scene_set_message(sc, NULL, 0);
}

void scene_reset(struct scene *sc, int term_w, int term_h) {
  entity_list_clear(&sc->entities);
  sc->castle_hidden_by = 0;

  if (sc->message_rows != NULL) add_message(sc, term_w, term_h);
  add_environment(sc, term_w, term_h);
  add_castle(sc, term_w, term_h);
  add_all_seaweed(sc, term_w, term_h);
  add_all_fish(sc, term_w, term_h);
  spawn_kaiju(sc, term_w, term_h);
  spawn_random_object(sc, term_w, term_h);
}

void scene_tick(struct scene *sc, int term_w, int term_h) {
  entity_tick_all(&sc->entities, term_w, term_h);
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    if (rng_int(100) > 97) {
      int fw = entity_width(fish);
      int fh = entity_height(fish);
      spawn_bubble(sc, fish->x, fish->y, fish->z, fw, fh, fish->vx);
    }
  }

  if (sc->castle_hidden_by == 0) {
    struct entity *kaiju_ent = NULL;
    struct entity *castle_ent = NULL;
    for (int i = 0; i < sc->entities.count; i++) {
      struct entity *e = &sc->entities.items[i];
      if (e->marked_dead) continue;
      if (e->type == ENT_KAIJU)
        kaiju_ent = e;
      else if (e->type == ENT_CASTLE)
        castle_ent = e;
    }
    if (kaiju_ent != NULL && castle_ent != NULL && entity_glyph_overlap(kaiju_ent, castle_ent)) {
      int kaiju_id = kaiju_ent->id;
      double castle_x = castle_ent->x;
      double castle_y = castle_ent->y;
      int castle_h = entity_height(castle_ent);
      castle_ent->marked_dead = true;
      spawn_rubble(sc, castle_x, castle_y, castle_h);
      sc->castle_hidden_by = kaiju_id;
    }
  }

  {
    struct entity *laser = NULL;
    for (int i = 0; i < sc->entities.count; i++) {
      struct entity *e = &sc->entities.items[i];
      if (!e->marked_dead && e->type == ENT_LASER) {
        laser = e;
        break;
      }
    }

    if (laser != NULL) {
      struct entity *target = NULL;
      for (int i = 0; i < sc->entities.count; i++) {
        struct entity *e = &sc->entities.items[i];
        if (!e->marked_dead && e->id == laser->splat_z) {
          target = e;
          break;
        }
      }
      if (target == NULL) {
        laser->marked_dead = true;
      } else {
        double d = target->x - laser->splat_x;
        double dist = d < 0 ? -d : d;
        int new_len = laser->age_ticks + 5; /* grow rate, cells/tick */
        if ((double)new_len >= dist) {
          spawn_splat(sc, target->x, target->y, target->z);
          target->marked_dead = true;
          target->death_action = DEATH_NONE;
          laser->marked_dead = true;
        } else {
          laser->age_ticks = new_len;
          update_laser_shape(laser, new_len);
        }
      }
    } else {
      struct entity *kaiju_ent = NULL;
      for (int i = 0; i < sc->entities.count; i++) {
        struct entity *e = &sc->entities.items[i];
        if (!e->marked_dead && e->type == ENT_KAIJU) {
          kaiju_ent = e;
          break;
        }
      }

      int eye_row, eye_col;
      if (kaiju_ent != NULL && find_kaiju_eye(kaiju_ent, &eye_row, &eye_col)) {
        double eye_x = kaiju_ent->x + eye_col;
        double eye_y = kaiju_ent->y + eye_row;
        double dir_sign = (kaiju_ent->vx >= 0) ? 1.0 : -1.0;

        for (int i = 0; i < sc->entities.count; i++) {
          struct entity *fish = &sc->entities.items[i];
          if (fish->marked_dead || fish->type != ENT_FISH) continue;
          if (fish->z != kaiju_ent->z) continue;
          int fh = entity_height(fish);
          if (!(eye_y >= fish->y && eye_y < fish->y + fh)) continue;
          bool ahead = (dir_sign > 0) ? (fish->x > eye_x) : (fish->x < eye_x);
          if (!ahead) continue;
          double d = fish->x - eye_x;
          double dist = d < 0 ? -d : d;
          if (dist >= term_w / 4.0) continue;
          spawn_laser(sc, eye_x, eye_y, kaiju_ent->z, fish->id, dir_sign);
          break;
        }
      }
    }
  }

  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *hook = &sc->entities.items[i];
    if (hook->marked_dead || hook->type != ENT_FISHHOOK) continue;
    if (hook->physical) {
      double resting = 0.75 * term_h - fishhook_body_height();
      if (hook->splat_x < resting) {
        hook->splat_x += 1.0;
        update_fishhook_shape(hook, (int)hook->splat_x);
      }
      for (int j = 0; j < sc->entities.count; j++) {
        struct entity *candidate = &sc->entities.items[j];
        if (candidate->marked_dead) continue;
        if (candidate->type != ENT_FISH && candidate->type != ENT_KAIJU) continue;
        if (!entity_glyph_overlap(hook, candidate)) continue;
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

  entity_collide_all(&sc->entities);
  bool shark_died = false;
  for (int i = 0; i < sc->entities.count; i++) {
    if (sc->entities.items[i].marked_dead &&
        sc->entities.items[i].type == ENT_SHARK) {
      shark_died = true;
      break;
    }
  }
  if (shark_died) {
    for (int i = 0; i < sc->entities.count; i++) {
      if (sc->entities.items[i].type == ENT_TEETH) sc->entities.items[i].marked_dead = true;
    }
  }

  if (sc->castle_hidden_by != 0) {
    bool this_kaiju_leaving = false;
    for (int i = 0; i < sc->entities.count; i++) {
      struct entity *e = &sc->entities.items[i];
      if (e->marked_dead && e->type == ENT_KAIJU && e->id == sc->castle_hidden_by) {
        this_kaiju_leaving = true;
        break;
      }
    }
    if (this_kaiju_leaving) {
      for (int i = 0; i < sc->entities.count; i++) {
        if (sc->entities.items[i].type == ENT_RUBBLE) sc->entities.items[i].marked_dead = true;
      }
    }
  }

  struct scene_ctx ctx = {sc, term_w, term_h};
  entity_reap(&sc->entities, on_death, &ctx);
}

void scene_set_message(struct scene *sc, const char *const *rows, int row_count) {
  if (sc->message_rows != NULL) {
    for (int i = 0; sc->message_rows[i] != NULL; i++) free(sc->message_rows[i]);
    free(sc->message_rows);
    sc->message_rows = NULL;
    sc->message_frame.shape = NULL;
  }

  if (rows == NULL || row_count <= 0) return;
  char **copy = malloc((size_t)(row_count + 1) * sizeof(*copy));
  for (int i = 0; i < row_count; i++) {
    size_t len = strlen(rows[i]);
    copy[i] = malloc(len + 1);
    memcpy(copy[i], rows[i], len + 1);
  }
  copy[row_count] = NULL;
  sc->message_rows = copy;
  sc->message_frame.shape = (ascii_rows)sc->message_rows;
  sc->message_frame.mask = NULL;
}

void scene_draw(const struct scene *sc, struct canvas *c) {
  entity_draw_all(&sc->entities, c);
}
