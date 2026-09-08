#ifndef UNDERTHEC_ENTITY_H
#define UNDERTHEC_ENTITY_H

#include <stdbool.h>

#include "canvas.h"
#include "sprite.h"

enum entity_type {
  ENT_WATERLINE,
  ENT_CASTLE,
  ENT_SEAWEED,
  ENT_FISH,
  ENT_BUBBLE,
  ENT_SPLAT,
  ENT_TEETH,
  ENT_SHARK,
  ENT_SHIP,
  ENT_WHALE,
  ENT_MONSTER,
  ENT_BIGFISH,
  ENT_MESSAGE,
  ENT_KAIJU,
  ENT_SUBMARINE,
  ENT_SWORDFISH,
  ENT_RUBBLE,
  ENT_LASER,
  ENT_DUCK,
  ENT_DOLPHIN,
  ENT_SWAN,
  ENT_FISHHOOK,
  ENT_CRAB,
  ENT_KAIJU_TIMER,
  ENT_JELLYFISH
};

enum death_action {
  DEATH_NONE,
  DEATH_ADD_FISH,
  DEATH_ADD_SEAWEED,
  DEATH_ADD_KAIJU,
  DEATH_RANDOM_OBJECT,
  DEATH_SHARK,
  DEATH_ADD_KAIJU_COOLDOWN
};

struct entity {
  int id;
  enum entity_type type;
  double x, y;
  int z;
  double vx, vy;
  const struct sprite_pair *frames;
  int frame_count;
  int frame_cur;
  double frame_interval;
  double frame_timer;

  char sentinel;
  bool trim_edges;
  struct attr default_attr;
  char **owned_mask;
  struct sprite_pair *owned_frame_table;
  char ***owned_shape_rows;
  bool physical;
  bool die_offscreen;
  int die_frame;
  int age_ticks;
  double die_after;
  enum death_action death_action;
  bool marked_dead;
  bool spawn_splat;
  double splat_x, splat_y;
  int splat_z;
};

struct entity_list {
  struct entity *items;
  int count;
  int capacity;
};

void entity_list_init(struct entity_list *list);
void entity_list_free(struct entity_list *list);
void entity_list_clear(struct entity_list *list);
struct entity *entity_spawn(struct entity_list *list);
struct entity *entity_find_first(struct entity_list *list, enum entity_type type);
struct entity *entity_find_by_id(struct entity_list *list, int id);
void entity_randomize_mask(struct entity *e, ascii_rows mask_template);
typedef void (*row_transform_fn)(const char *in, char *out, void *ctx);
char **entity_build_transformed_rows(ascii_rows tmpl, row_transform_fn fn, void *ctx);
void entity_free_owned_rows(char **rows);
void entity_set_owned_shape_frames(struct entity *e, char ***rows, int frame_count, double frame_interval_ticks);
void entity_set_owned_single_row(struct entity *e, char *row, double frame_interval_ticks);
void entity_clear_owned(struct entity *e);
ascii_rows entity_shape(const struct entity *e);
ascii_rows entity_mask(const struct entity *e);
int entity_width(const struct entity *e);
int entity_height(const struct entity *e);
void entity_tick_all(struct entity_list *list, int term_w, int term_h);
void entity_collide_all(struct entity_list *list);
bool entity_glyph_overlap(const struct entity *a, const struct entity *b);
typedef void (*entity_death_fn)(const struct entity *dead, void *ctx);
void entity_reap(struct entity_list *list, entity_death_fn fn, void *ctx);
void entity_draw_all(const struct entity_list *list, struct canvas *c);
void entity_draw_shutdown(void);

#endif
