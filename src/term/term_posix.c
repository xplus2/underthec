#define _DEFAULT_SOURCE

#include "../color.h"
#include "term.h"

#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int tty_fd = -1;

static struct termios orig_termios;
static bool have_orig_termios = false;
static bool is_active = false;
static char stdout_buf[1 << 16];

static struct canvas prev;
static bool prev_valid = false;

int term_init(void) {
  tty_fd = open("/dev/tty", O_RDWR);
  if (tty_fd < 0) return -1;
  if (tcgetattr(tty_fd, &orig_termios) != 0) return -1;
  have_orig_termios = true;
  struct termios raw = orig_termios;
  cfmakeraw(&raw);
  raw.c_lflag |= ISIG; /* keep ctrl+c/ctrl+\ generating SIGINT/SIGQUIT */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(tty_fd, TCSAFLUSH, &raw) != 0) return -1;
  setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));
  fputs("\x1b[?1049h\x1b[?25l\x1b[2J\x1b[H", stdout);
  fflush(stdout);
  canvas_init(&prev);
  prev_valid = false;
  is_active = true;
  return 0;
}

void term_shutdown(void) {
  if (!is_active) return;
  is_active = false;
  fputs("\x1b[?25h\x1b[?1049l", stdout);
  fflush(stdout);
  if (have_orig_termios) tcsetattr(tty_fd, TCSAFLUSH, &orig_termios);
  if (tty_fd >= 0) {
    close(tty_fd);
    tty_fd = -1;
  }
  canvas_free(&prev);
}

void term_size(int *cols, int *rows) {
  struct winsize ws;
  if (ioctl(tty_fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return;
  }
  *cols = 80;
  *rows = 24;
}

bool term_has_color(void) { return color_supported(); }

int term_poll_key(int timeout_ms) {
  struct pollfd pfd = {.fd = tty_fd, .events = POLLIN};
  int ret = poll(&pfd, 1, timeout_ms);
  if (ret <= 0) return -1;
  unsigned char c;
  if (read(tty_fd, &c, 1) != 1) return -1;
  return tolower(c);
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
      struct cell *cur = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
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
