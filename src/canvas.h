#ifndef UNDERTHEC_CANVAS_H
#define UNDERTHEC_CANVAS_H

#include "color.h"

struct cell {
  unsigned char glyph;
  enum color col;
  bool bold;
};

struct canvas {
  int width;
  int height;
  struct cell *cells;
};

/* w/h may start being 0 */
void canvas_init(struct canvas *c);
void canvas_free(struct canvas *c);
void canvas_resize(struct canvas *c, int width, int height);
void canvas_clear(struct canvas *c);
void canvas_put(struct canvas *c, int x, int y, unsigned char glyph, struct attr a);

#endif
