#include "../color.h"
#include "term.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static HANDLE h_in;
static HANDLE h_out;
static DWORD orig_in_mode;
static DWORD orig_out_mode;
static bool have_orig_modes = false;
static bool vt_enabled = false;
static bool is_active = false;
static char stdout_buf[1 << 16];

static struct canvas prev;
static bool prev_valid = false;

int term_init(void) {
  h_in = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  h_out = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h_in == INVALID_HANDLE_VALUE || h_out == INVALID_HANDLE_VALUE) return -1;
  if (!GetConsoleMode(h_in, &orig_in_mode) || !GetConsoleMode(h_out, &orig_out_mode)) return -1;
  have_orig_modes = true;
  DWORD out_mode = orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(h_out, out_mode)) return -1;
  vt_enabled = true;
  DWORD in_mode = orig_in_mode;
  in_mode &= ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
  in_mode |= ENABLE_PROCESSED_INPUT;
  if (!SetConsoleMode(h_in, in_mode)) return -1;
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
  if (have_orig_modes) {
    SetConsoleMode(h_in, orig_in_mode);
    SetConsoleMode(h_out, orig_out_mode);
  }
  if (h_in != INVALID_HANDLE_VALUE) {
    CloseHandle(h_in);
    h_in = INVALID_HANDLE_VALUE;
  }
  canvas_free(&prev);
}

void term_size(int *cols, int *rows) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(h_out, &csbi)) {
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if (*cols > 0 && *rows > 0) return;
  }
  *cols = 80;
  *rows = 24;
}

bool term_has_color(void) { return vt_enabled && color_supported(); }

int term_poll_key(int timeout_ms) {
  DWORD waited = WaitForSingleObject(h_in, (DWORD)timeout_ms);
  if (waited != WAIT_OBJECT_0) return -1;
  INPUT_RECORD rec;
  DWORD read = 0;
  if (!ReadConsoleInputA(h_in, &rec, 1, &read) || read == 0) return -1;
  if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
    char c = rec.Event.KeyEvent.uChar.AsciiChar;
    if (c != 0) return tolower((unsigned char)c);
  }
  return -1;
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
