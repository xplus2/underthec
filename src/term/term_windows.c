#include "../color.h"
#include "term.h"

#include <ctype.h>
#include <stdio.h>
#include <windows.h>

static HANDLE h_in;
static HANDLE h_out;
static DWORD orig_in_mode;
static DWORD orig_out_mode;
static bool have_orig_modes = false;
static bool vt_enabled = false;
static bool is_active = false;
static char stdout_buf[1 << 16];

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
  term_common_shutdown();
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
