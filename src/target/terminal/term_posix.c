#define _DEFAULT_SOURCE

#include "../../color.h"
#include "term.h"

#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int tty_fd = -1;

static struct termios orig_termios;
static bool have_orig_termios = false;
static bool is_active = false;
static char stdout_buf[1 << 16];

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
  term_common_shutdown();
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

static bool have_pending = false;
static unsigned char pending_byte;

static bool read_byte(int timeout_ms, unsigned char *out) {
  if (have_pending) {
    *out = pending_byte;
    have_pending = false;
    return true;
  }
  struct pollfd pfd = {.fd = tty_fd, .events = POLLIN};
  if (poll(&pfd, 1, timeout_ms) <= 0) return false;
  return read(tty_fd, out, 1) == 1;
}

int term_poll_key(int timeout_ms) {
  unsigned char c;
  if (!read_byte(timeout_ms, &c)) return -1;
  if (c != 0x1b) return tolower(c);

  unsigned char c2;
  if (!read_byte(25, &c2)) return 27;
  if (c2 != '[') {
    have_pending = true;
    pending_byte = c2;
    return 27;
  }
  unsigned char c3;
  if (!read_byte(25, &c3)) return 27;
  switch (c3) {
    case 'A': return TERM_KEY_UP;
    case 'B': return TERM_KEY_DOWN;
    case 'C': return TERM_KEY_RIGHT;
    case 'D': return TERM_KEY_LEFT;
    default:  return -1;
  }
}
