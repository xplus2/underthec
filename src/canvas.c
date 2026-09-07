#include "canvas.h"

#include <stdlib.h>
#include <string.h>

void canvas_init(struct canvas *c) {
  c->width = 0;
  c->height = 0;
  c->cells = NULL;
}

void canvas_free(struct canvas *c) {
  free(c->cells);
  c->cells = NULL;
  c->width = 0;
  c->height = 0;
}

void canvas_resize(struct canvas *c, int width, int height) {
  if (width < 0)  width = 0;
  if (height < 0) height = 0;
  if (width == c->width && height == c->height) return;

  free(c->cells);
  c->cells = calloc((size_t)width * (size_t)height, sizeof(*c->cells));
  c->width = width;
  c->height = height;
  canvas_clear(c);
}

void canvas_clear(struct canvas *c) {
  struct cell blank = {' ', COL_DEFAULT, false};
  int n = c->width * c->height;
  for (int i = 0; i < n; i++) c->cells[i] = blank;
}

void canvas_put(struct canvas *c, int x, int y, unsigned char glyph, struct attr a) {
  if (x < 0 || y < 0 || x >= c->width || y >= c->height) return;
  struct cell *cell = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
  cell->glyph = glyph;
  cell->col = a.col;
  cell->bold = a.bold;
}
