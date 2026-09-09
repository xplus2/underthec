#include "../color.h"
#include "term.h"

#include <stdio.h>
#include <string.h>

static struct canvas prev;
static bool prev_valid = false;
static bool transparent = false;

void term_common_shutdown(void) {
  canvas_free(&prev);
  prev_valid = false;
}

void term_set_transparent(bool on) {
  transparent = on;
  prev_valid = false; /* repaint bg */
}

static const char *const sgr_table[2][2][9] = {
  {
    {"\x1b[0;39;40m", "\x1b[0;30;40m", "\x1b[0;31;40m", "\x1b[0;32;40m", "\x1b[0;33;40m",
     "\x1b[0;34;40m", "\x1b[0;35;40m", "\x1b[0;36;40m", "\x1b[0;37;40m"},
    {"\x1b[0;1;39;40m", "\x1b[0;1;30;40m", "\x1b[0;1;31;40m", "\x1b[0;1;32;40m", "\x1b[0;1;33;40m",
     "\x1b[0;1;34;40m", "\x1b[0;1;35;40m", "\x1b[0;1;36;40m", "\x1b[0;1;37;40m"},
  },
  {
    {"\x1b[0;39m", "\x1b[0;30m", "\x1b[0;31m", "\x1b[0;32m", "\x1b[0;33m",
     "\x1b[0;34m", "\x1b[0;35m", "\x1b[0;36m", "\x1b[0;37m"},
    {"\x1b[0;1;39m", "\x1b[0;1;30m", "\x1b[0;1;31m", "\x1b[0;1;32m", "\x1b[0;1;33m",
     "\x1b[0;1;34m", "\x1b[0;1;35m", "\x1b[0;1;36m", "\x1b[0;1;37m"},
  },
};

static void write_sgr(enum color col, bool bold, bool mono) {
  if (mono) return;
  fputs(sgr_table[transparent][bold][col], stdout);
}

static char *append_uint(char *p, unsigned v) {
  char tmp[10];
  int n = 0;
  do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
  while (n > 0) *p++ = tmp[--n];
  return p;
}

static void write_cursor_pos(int row, int col) {
  char buf[24];
  char *p = buf;
  *p++ = '\x1b';
  *p++ = '[';
  p = append_uint(p, (unsigned)row);
  *p++ = ';';
  p = append_uint(p, (unsigned)col);
  *p++ = 'H';
  fwrite(buf, 1, (size_t)(p - buf), stdout);
}

static bool cells_equal(const struct cell *a, const struct cell *b) {
  return a->cont == b->cont && a->col == b->col && a->bold == b->bold && strcmp(a->glyph, b->glyph) == 0;
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
      if (cells_equal(cur, old)) {
        x++;
        continue;
      }
      int start = (cur->cont && x > 0) ? x - 1 : x;
      write_cursor_pos(y + 1, start + 1);
      x = start;
      enum color last_col = COL_DEFAULT;
      bool last_bold = false;
      bool first = true;
      while (x < c->width) {
        cur = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
        old = &prev.cells[(size_t)y * (size_t)c->width + (size_t)x];
        if (cur->cont) {
          *old = *cur;
          x++;
          continue;
        }
        if (x != start && cells_equal(cur, old)) break;
        if (first || cur->col != last_col || cur->bold != last_bold) {
          write_sgr(cur->col, cur->bold, mono);
          last_col = cur->col;
          last_bold = cur->bold;
          first = false;
        }
        fputs(cur->glyph, stdout);
        *old = *cur;
        x++;
      }
    }
  }
  fflush(stdout);
}
