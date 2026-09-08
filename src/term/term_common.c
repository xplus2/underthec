#include "../color.h"
#include "term.h"

#include <stdio.h>
#include <string.h>

static struct canvas prev;
static bool prev_valid = false;

void term_common_shutdown(void) {
  canvas_free(&prev);
  prev_valid = false;
}

static void write_sgr(enum color col, bool bold, bool mono) {
  if (mono) return;
  static const int fg[] = {39, 30, 31, 32, 33, 34, 35, 36, 37};
  printf("\x1b[0;%s%dm", bold ? "1;" : "", fg[col]);
}

void term_present(const struct canvas *c) {
  bool mono = !term_has_color();
  if (!prev_valid || prev.width != c->width || prev.height != c->height) {
    canvas_resize(&prev, c->width, c->height);
    memset(prev.cells, 0, (size_t)prev.width * (size_t)prev.height * sizeof(*prev.cells));
    fputs("\x1b[2J", stdout);
    prev_valid = true;
  }

  for (int y = 0; y < c->height; y++) {
    int x = 0;
    while (x < c->width) {
      const struct cell *cur = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
      struct cell *old = &prev.cells[(size_t)y * (size_t)c->width + (size_t)x];
      if (cur->glyph == old->glyph && cur->col == old->col && cur->bold == old->bold) {
        x++;
        continue;
      }
      printf("\x1b[%d;%dH", y + 1, x + 1);
      enum color last_col = COL_DEFAULT;
      bool last_bold = false;
      bool first = true;
      while (x < c->width) {
        cur = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
        old = &prev.cells[(size_t)y * (size_t)c->width + (size_t)x];
        if (cur->glyph == old->glyph && cur->col == old->col && cur->bold == old->bold) break;
        if (first || cur->col != last_col || cur->bold != last_bold) {
          write_sgr(cur->col, cur->bold, mono);
          last_col = cur->col;
          last_bold = cur->bold;
          first = false;
        }
        putchar((int)cur->glyph);
        *old = *cur;
        x++;
      }
    }
  }
  fflush(stdout);
}
