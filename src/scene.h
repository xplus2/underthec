#ifndef UNDERTHEC_SCENE_H
#define UNDERTHEC_SCENE_H

#include "entity.h"

struct scene {
  struct entity_list entities;
  bool classic_mode;
  char **message_rows;
  struct sprite_pair message_frame;
  int castle_hidden_by;
};

void scene_init(struct scene *sc, bool classic_mode);
void scene_free(struct scene *sc);
void scene_reset(struct scene *sc, int term_w, int term_h);
void scene_tick(struct scene *sc, int term_w, int term_h);
void scene_set_message(struct scene *sc, const char *const *rows, int row_count);
void scene_draw(const struct scene *sc, struct canvas *c);

#endif
