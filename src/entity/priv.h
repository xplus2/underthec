#ifndef UNDERTHEC_ENTITY_PRIV_H
#define UNDERTHEC_ENTITY_PRIV_H

#include "entity.h"

static inline int round_to_int(double v) {
  return (int)(v >= 0.0 ? v + 0.5 : v - 0.5);
}

int utf8_seq_len(unsigned char lead);
int utf8_char_width(const char *s, int seq_len);
int utf8_col_width(const char *s);
int utf8_byte_offset(const char *s, int col);

bool cell_transparent(const struct entity *e, const char *srow, int len, int col);

void tick_seaweed_growth(struct entity *e);
void tick_seaweed_debris(struct entity *e, int term_h);

#endif
