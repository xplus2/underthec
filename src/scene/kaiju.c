#include "scene_internal.h"
#include "color.h"
#include "rng.h"

#include "art/kaiju.h"

#include <stdlib.h>
#include <string.h>

struct kaiju_mask_ctx {
  char body;
  char back;
};

static void map_kaiju_colors(const char *in, char *out, void *ctx) {
  struct kaiju_mask_ctx *k = ctx;
  size_t len = strlen(in);
  for (size_t j = 0; j < len; j++) {
    char ch = in[j];
    if (ch == '2')       out[j] = k->body;
    else if (ch == '3')  out[j] = k->back;
    else                 out[j] = ch;
  }
  out[len] = '\0';
}

static void randomize_kaiju_mask(struct entity *e, ascii_rows tmpl) {
  static const char body_letters[4] = {'G', 'g', 'w', 'K'};
  static const char back_letters[5] = {'c', 'y', 'Y', 'R', 'r'};
  struct kaiju_mask_ctx ctx = {body_letters[rng_int(4)], back_letters[rng_int(5)]};
  e->owned_mask = entity_build_transformed_rows(tmpl, map_kaiju_colors, &ctx);
}

void spawn_kaiju(struct scene *sc, int w, int h) {
  int dir = rng_int(2);
  double speed = rng_double(2.0) + 0.25;
  if (dir) speed = -speed;

  struct entity *e = entity_spawn(&sc->entities);
  e->frames = &kaiju[dir];
  e->frame_count = 1;
  randomize_kaiju_mask(e, kaiju[dir].mask);
  int width = entity_width(e);
  int height = entity_height(e);
  e->y = random_swim_y(h, height);
  e->x = dir ? (double)(w - 2) : (double)(1 - width);

  finish_creature_spawn(e, ENT_KAIJU, rng_int(Z_FISH_RANGE) + Z_FISH_MIN, speed, 0, DEATH_ADD_KAIJU_COOLDOWN, (struct attr){COL_DEFAULT, false});
}

void schedule_kaiju_return(struct scene *sc) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_KAIJU_TIMER;
  e->die_after = rng_double(10.0) + 5.0;
  e->death_action = DEATH_ADD_KAIJU;
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
  entity_set_owned_single_row(e, line, 0.0);

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

static void handle_castle_collision(struct scene *sc) {
  if (sc->castle_hidden_by != 0) return;
  struct entity *kaiju_ent = entity_find_first(&sc->entities, ENT_KAIJU);
  struct entity *castle_ent = entity_find_first(&sc->entities, ENT_CASTLE);
  if (kaiju_ent == NULL || castle_ent == NULL) return;
  if (!entity_glyph_overlap(kaiju_ent, castle_ent)) return;

  int kaiju_id = kaiju_ent->id;
  double castle_x = castle_ent->x;
  double castle_y = castle_ent->y;
  int castle_h = entity_height(castle_ent);
  castle_ent->marked_dead = true;
  spawn_rubble(sc, castle_x, castle_y, castle_h);
  sc->castle_hidden_by = kaiju_id;
}

static void update_active_laser(struct scene *sc, struct entity *laser) {
  struct entity *target = entity_find_by_id(&sc->entities, laser->splat_z);
  struct entity *kaiju_ent = entity_find_first(&sc->entities, ENT_KAIJU);

  int eye_row, eye_col;
  if (kaiju_ent != NULL && find_kaiju_eye(kaiju_ent, &eye_row, &eye_col)) {
    laser->splat_x = kaiju_ent->x + eye_col;
    laser->y = kaiju_ent->y + eye_row;
  }

  if (target == NULL) {
    laser->marked_dead = true;
    return;
  }

  double d = target->x - laser->splat_x;
  double dist = d < 0 ? -d : d;
  int new_len = laser->age_ticks + 5;
  if ((double)new_len >= dist) {
    spawn_splat(sc, target->x, target->y, target->z);
    target->marked_dead = true;
    laser->marked_dead = true;
    return;
  }
  laser->age_ticks = new_len;
  update_laser_shape(laser, new_len);
}

static void fire_laser_if_ready(struct scene *sc, int term_w) {
  struct entity *kaiju_ent = entity_find_first(&sc->entities, ENT_KAIJU);
  if (kaiju_ent == NULL) return;

  int eye_row, eye_col;
  if (!find_kaiju_eye(kaiju_ent, &eye_row, &eye_col)) return;
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

static void handle_laser(struct scene *sc, int term_w) {
  struct entity *laser = entity_find_first(&sc->entities, ENT_LASER);
  if (laser != NULL)
    update_active_laser(sc, laser);
  else
    fire_laser_if_ready(sc, term_w);
}

static void handle_castle_reveal(struct scene *sc) {
  if (sc->castle_hidden_by == 0) return;

  bool leaving = false;
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *e = &sc->entities.items[i];
    if (e->marked_dead && e->type == ENT_KAIJU && e->id == sc->castle_hidden_by) {
      leaving = true;
      break;
    }
  }
  if (!leaving) return;

  for (int i = 0; i < sc->entities.count; i++) {
    if (sc->entities.items[i].type == ENT_RUBBLE) sc->entities.items[i].marked_dead = true;
  }
}

void kaiju_tick(struct scene *sc, int term_w) {
  handle_castle_collision(sc);
  handle_laser(sc, term_w);
  handle_castle_reveal(sc);
}
