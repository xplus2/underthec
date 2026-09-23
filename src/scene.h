#ifndef UNDERTHEC_SCENE_H
#define UNDERTHEC_SCENE_H

#include <stddef.h>

#include "color.h"
#include "entity.h"

/* creature flags, excl. fish_count */
#define SCENE_AQUATIC_FLAG_COUNT 15

struct aquatic_life {
  int fish_count; /* < 0 = auto, screen-size based */
  bool ducks;
  bool dolphins;
  bool ship;
  bool swan;
  bool kaiju;
  bool fishhook;
  bool submarine;
  bool whale;
  bool shark;
  bool jellyfish;
  bool seahorse;
  bool monster;
  bool bigfish;
  bool swordfish;
  bool crab;
};

enum message_position {
  MSG_POS_MIDDLE,
  MSG_POS_CENTER,
  MSG_POS_MARQUEE,
  MSG_POS_SWIM,
  MSG_POS_EVENT
};

struct scene {
  struct entity_list entities;
  bool classic_mode;
  struct aquatic_life aquatic;
  char **message_rows;
  struct sprite_pair message_frame;
  struct attr message_attr;
  enum message_position message_position;
  bool castle;
  int castle_hidden_by;
  int uturn_chance;
  bool feed_alerted;
};

/* all on, fish=auto */
struct aquatic_life scene_aquatic_default(void);
/* classic 1.1 lineup */
struct aquatic_life scene_aquatic_classic11(void);
/* creature flags only, fish_count untouched */
void scene_aquatic_fill(struct aquatic_life *a, bool on);
/* false if name unknown */
bool scene_aquatic_set_flag(struct aquatic_life *a, const char *name);
const char *scene_aquatic_flag_name(size_t i);
bool *scene_aquatic_flag(struct aquatic_life *a, size_t i);

void scene_init(struct scene *sc, bool classic_mode, struct aquatic_life aquatic);
void scene_free(struct scene *sc);
void scene_reset(struct scene *sc, int term_w, int term_h);
void scene_tick(struct scene *sc, int term_w, int term_h);
void scene_set_message(struct scene *sc, const char *const *rows, int row_count);
void scene_set_message_color(struct scene *sc, struct attr attr);
void scene_set_message_position(struct scene *sc, enum message_position pos);
void scene_set_uturn_chance(struct scene *sc, int one_in);
/* next reset */
void scene_set_castle(struct scene *sc, bool on);
void scene_feed(struct scene *sc, int w, int h);
void scene_set_fish_count(struct scene *sc, int w, int h, int count);
int scene_fish_display_count(const struct scene *sc);
void scene_on_species_toggled(struct scene *sc, int w, int h);
void scene_draw(const struct scene *sc, struct canvas *c, double alpha);

#endif
