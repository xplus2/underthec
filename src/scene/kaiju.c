#include "scene_internal.h"
#include "color.h"
#include "rng.h"

#include "art/kaiju.h"

#include <stdlib.h>
#include <string.h>

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

void spawn_kaiju(struct scene *sc, int w, int h) {
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

void kaiju_tick(struct scene *sc, int term_w) {
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
      struct entity *kaiju_ent = NULL;
      for (int i = 0; i < sc->entities.count; i++) {
        struct entity *e = &sc->entities.items[i];
        if (e->marked_dead)
          continue;
        if (e->id == laser->splat_z)
          target = e;
        else if (e->type == ENT_KAIJU)
          kaiju_ent = e;
      }
      int eye_row, eye_col;
      if (kaiju_ent != NULL && find_kaiju_eye(kaiju_ent, &eye_row, &eye_col)) {
        laser->splat_x = kaiju_ent->x + eye_col;
        laser->y = kaiju_ent->y + eye_row;
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
}
