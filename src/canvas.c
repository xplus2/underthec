#include "canvas.h"
#include "xalloc.h"

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
  size_t n = (size_t)width * (size_t)height;
  c->cells = n > 0 ? xcalloc(n, sizeof(*c->cells)) : NULL;
  c->width = width;
  c->height = height;
  canvas_clear(c);
}

void canvas_clear(struct canvas *c) {
  struct cell blank = {{' ', '\0'}, COL_DEFAULT, false};
  int n = c->width * c->height;
  for (int i = 0; i < n; i++) c->cells[i] = blank;
}

void canvas_put(struct canvas *c, int x, int y, const char *glyph, int glyph_len, struct attr a) {
  if (x < 0 || y < 0 || x >= c->width || y >= c->height) return;
  if (glyph_len < 1) glyph_len = 1;
  if (glyph_len > 4) glyph_len = 4;
  struct cell *cell = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
  memcpy(cell->glyph, glyph, (size_t)glyph_len);
  cell->glyph[glyph_len] = '\0';
  cell->col = a.col;
  cell->bold = a.bold;
}
