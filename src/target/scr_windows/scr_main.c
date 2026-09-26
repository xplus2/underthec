#include "scr.h"
#include "../../app.h"
#include "../../config.h"
#include "../../version.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

#define RUN_MAX 512
#define MOUSE_SLACK 8
#define TIMER_ID 1

/* enum color order, [normal, bold] */
static const COLORREF palette[9][2] = {
  {RGB(0xc0, 0xc0, 0xc0), RGB(0xff, 0xff, 0xff)},
  {RGB(0x00, 0x00, 0x00), RGB(0x80, 0x80, 0x80)},
  {RGB(0xc0, 0x00, 0x00), RGB(0xff, 0x55, 0x55)},
  {RGB(0x00, 0xc0, 0x00), RGB(0x55, 0xff, 0x55)},
  {RGB(0xc0, 0xc0, 0x00), RGB(0xff, 0xff, 0x55)},
  {RGB(0x00, 0x00, 0xc0), RGB(0x55, 0x55, 0xff)},
  {RGB(0xc0, 0x00, 0xc0), RGB(0xff, 0x55, 0xff)},
  {RGB(0x00, 0xc0, 0xc0), RGB(0x55, 0xff, 0xff)},
  {RGB(0xc0, 0xc0, 0xc0), RGB(0xff, 0xff, 0xff)},
};

struct scr {
  struct app app;
  bool started;
  char err[256]; /* set: shown instead of tank */
  bool fullscreen;
  HFONT font[2]; /* [normal, bold] */
  int font_px;
  int cell_w;
  int cell_h;
  int off_x;
  int off_y;
  HDC mem_dc;
  HBITMAP mem_bmp;
  HGDIOBJ old_bmp;
  int bmp_w;
  int bmp_h;
  bool mouse_seen;
  POINT mouse_origin;
  LARGE_INTEGER freq;
};

static struct scr scr;

static double now_seconds(void) {
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return (double)t.QuadPart / (double)scr.freq.QuadPart;
}

static void fonts_create(HDC dc) {
  for (int b = 0; b < 2; b++) {
    scr.font[b] = CreateFontW(-scr.font_px, 0, 0, 0, b ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
  }
  HGDIOBJ old = SelectObject(dc, scr.font[0]);
  TEXTMETRICW tm;
  SIZE sz;
  GetTextMetricsW(dc, &tm);
  GetTextExtentPoint32W(dc, L"M", 1, &sz);
  SelectObject(dc, old);
  scr.cell_w = sz.cx > 0 ? sz.cx : 1;
  scr.cell_h = tm.tmHeight > 0 ? tm.tmHeight : 1;
}

static void fonts_free(void) {
  for (int b = 0; b < 2; b++) {
    if (scr.font[b] != NULL) DeleteObject(scr.font[b]);
    scr.font[b] = NULL;
  }
}

static void backbuffer_free(void) {
  if (scr.mem_dc == NULL) return;
  SelectObject(scr.mem_dc, scr.old_bmp);
  DeleteObject(scr.mem_bmp);
  DeleteDC(scr.mem_dc);
  scr.mem_dc = NULL;
}

static void layout(HWND wnd, int w, int h) {
  if (w <= 0 || h <= 0) return;
  HDC dc = GetDC(wnd);
  if (scr.font[0] == NULL) fonts_create(dc);
  backbuffer_free();
  scr.mem_dc = CreateCompatibleDC(dc);
  scr.mem_bmp = CreateCompatibleBitmap(dc, w, h);
  scr.old_bmp = SelectObject(scr.mem_dc, scr.mem_bmp);
  ReleaseDC(wnd, dc);
  SetBkMode(scr.mem_dc, TRANSPARENT);
  scr.bmp_w = w;
  scr.bmp_h = h;
  int cols = w / scr.cell_w;
  int rows = h / scr.cell_h;
  if (cols < 1) cols = 1;
  if (rows < 1) rows = 1;
  scr.off_x = (w - cols * scr.cell_w) / 2;
  scr.off_y = (h - rows * scr.cell_h) / 2;
  if (scr.started) app_resize(&scr.app, cols, rows);
}

static void draw_run(const WCHAR *text, INT *dx, int n, int x, int y, const struct cell *attr) {
  if (n == 0) return;
  int b = attr->bold ? 1 : 0;
  SelectObject(scr.mem_dc, scr.font[b]);
  SetTextColor(scr.mem_dc, palette[attr->col][b]);
  ExtTextOutW(scr.mem_dc, x, y, 0, NULL, text, (UINT)n, dx);
}

/* runs of same fg attr per row, fixed advance */
static void draw_row(int row) {
  const struct canvas *c = &scr.app.canvas;
  const struct cell *line = &c->cells[(size_t)row * (size_t)c->width];
  int y = scr.off_y + row * scr.cell_h;
  WCHAR text[RUN_MAX];
  INT dx[RUN_MAX];
  int n = 0;
  int run_x = scr.off_x;
  const struct cell *run_attr = NULL;
  for (int col = 0; col < c->width; col++) {
    const struct cell *cell = &line[col];
    if (cell->cont) {
      if (n > 0) dx[n - 1] += scr.cell_w;
      continue;
    }
    bool same = run_attr != NULL && run_attr->col == cell->col && run_attr->bold == cell->bold;
    if (!same || n + 2 > RUN_MAX) {
      if (run_attr != NULL) draw_run(text, dx, n, run_x, y, run_attr);
      n = 0;
      run_x = scr.off_x + col * scr.cell_w;
      run_attr = cell;
    }
    int units = MultiByteToWideChar(CP_UTF8, 0, cell->glyph, -1, &text[n], 2) - 1;
    if (units <= 0) {
      text[n] = L' ';
      units = 1;
    }
    dx[n] = scr.cell_w;
    if (units == 2) dx[n + 1] = 0;
    n += units;
  }
  if (run_attr != NULL) draw_run(text, dx, n, run_x, y, run_attr);
}

static void draw_canvas(void) {
  const struct canvas *c = &scr.app.canvas;
  for (int row = 0; row < c->height; row++) {
    for (int col = 0; col < c->width; col++) {
      const struct cell *cell = &c->cells[(size_t)row * (size_t)c->width + (size_t)col];
      if (cell->bg == COL_DEFAULT) continue;
      RECT r = {scr.off_x + col * scr.cell_w, scr.off_y + row * scr.cell_h, 0, 0};
      r.right = r.left + scr.cell_w;
      r.bottom = r.top + scr.cell_h;
      HBRUSH br = CreateSolidBrush(palette[cell->bg][cell->bg_bold ? 1 : 0]);
      FillRect(scr.mem_dc, &r, br);
      DeleteObject(br);
    }
  }
  for (int row = 0; row < c->height; row++) draw_row(row);
}

static void draw_error(void) {
  WCHAR msg[300];
  int n = MultiByteToWideChar(CP_UTF8, 0, scr.err, -1, msg, (int)(sizeof msg / sizeof msg[0]));
  if (n <= 0) return;
  RECT r = {scr.cell_w, scr.cell_h, scr.bmp_w - scr.cell_w, scr.bmp_h - scr.cell_h};
  SelectObject(scr.mem_dc, scr.font[0]);
  SetTextColor(scr.mem_dc, palette[0][0]);
  DrawTextW(scr.mem_dc, msg, -1, &r, DT_WORDBREAK | DT_NOPREFIX);
}

static void paint(HWND wnd) {
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(wnd, &ps);
  if (scr.mem_dc != NULL) {
    RECT all = {0, 0, scr.bmp_w, scr.bmp_h};
    FillRect(scr.mem_dc, &all, GetStockObject(BLACK_BRUSH));
    if (scr.err[0] != '\0') draw_error();
    else if (scr.started) draw_canvas();
    BitBlt(dc, 0, 0, scr.bmp_w, scr.bmp_h, scr.mem_dc, 0, 0, SRCCOPY);
  }
  EndPaint(wnd, &ps);
}

static bool mouse_moved(LPARAM lp) {
  POINT p = {(short)LOWORD(lp), (short)HIWORD(lp)};
  if (!scr.mouse_seen) {
    scr.mouse_seen = true;
    scr.mouse_origin = p;
    return false;
  }
  return abs(p.x - scr.mouse_origin.x) > MOUSE_SLACK || abs(p.y - scr.mouse_origin.y) > MOUSE_SLACK;
}

static LRESULT CALLBACK wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_SIZE:
      layout(wnd, LOWORD(lp), HIWORD(lp));
      return 0;
    case WM_TIMER:
      if (scr.started) app_frame(&scr.app, now_seconds());
      InvalidateRect(wnd, NULL, FALSE);
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT:
      paint(wnd);
      return 0;
    case WM_DESTROY:
      KillTimer(wnd, TIMER_ID);
      PostQuitMessage(0);
      return 0;
    default:
      break;
  }
  if (scr.fullscreen) {
    switch (msg) {
      case WM_SETCURSOR:
        SetCursor(NULL);
        return TRUE;
      case WM_MOUSEMOVE:
        if (mouse_moved(lp)) PostMessageW(wnd, WM_CLOSE, 0, 0);
        return 0;
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_XBUTTONDOWN:
        PostMessageW(wnd, WM_CLOSE, 0, 0);
        return 0;
      case WM_ACTIVATEAPP:
        if (!wp) PostMessageW(wnd, WM_CLOSE, 0, 0);
        break;
      case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_SCREENSAVE) return 0;
        break;
      default:
        break;
    }
  }
  return DefWindowProcW(wnd, msg, wp, lp);
}

/* physical px with per mon DPI */
static BOOL CALLBACK add_monitor(HMONITOR mon, HDC dc, LPRECT r, LPARAM all) {
  (void)mon;
  (void)dc;
  UnionRect((RECT *)all, (RECT *)all, r);
  return TRUE;
}

/* preview: parent = preview host, NULL: fullscreen */
static int run(HINSTANCE inst, HWND parent) {
  QueryPerformanceFrequency(&scr.freq);
  scr.fullscreen = parent == NULL;
  struct config cfg;
  config_init(&cfg);
  if (scr_config_load(&cfg, scr.err, sizeof scr.err) && config_check(&cfg, scr.err, sizeof scr.err)) {
    config_start(&cfg, &scr.app, now_seconds());
    /* castle doesn't fit the preview */
    if (!scr.fullscreen) scene_set_castle(&scr.app.scene, false);
    scr.started = true;
  }
  int fps = cfg.fps;
  config_free(&cfg);
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = inst;
  wc.lpszClassName = L"underthec_scr";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassW(&wc);

  HWND wnd;
  if (scr.fullscreen) {
    RECT all = {0, 0, 0, 0};
    EnumDisplayMonitors(NULL, NULL, add_monitor, (LPARAM)&all);
    MONITORINFO primary = {.cbSize = sizeof primary};
    GetMonitorInfoW(MonitorFromPoint((POINT){0, 0}, MONITOR_DEFAULTTOPRIMARY), &primary);
    scr.font_px = (primary.rcMonitor.bottom - primary.rcMonitor.top) / 45;
    if (scr.font_px < 10) scr.font_px = 10;
    wnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, TOOL_DISPLAY_NAME_W, WS_POPUP, all.left, all.top, all.right - all.left,
                          all.bottom - all.top, NULL, NULL, inst, NULL);
  } else {
    RECT r;
    GetClientRect(parent, &r);
    scr.font_px = r.bottom / 14;
    if (scr.font_px < 6) scr.font_px = 6;
    wnd = CreateWindowExW(0, wc.lpszClassName, TOOL_DISPLAY_NAME_W, WS_CHILD, 0, 0, r.right, r.bottom, parent, NULL, inst, NULL);
  }
  if (wnd == NULL) return 1;
  ShowWindow(wnd, SW_SHOW);
  SetTimer(wnd, TIMER_ID, (UINT)(1000 / fps), NULL);

  MSG m;
  while (GetMessageW(&m, NULL, 0, 0) > 0) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  backbuffer_free();
  fonts_free();
  if (scr.started) app_free(&scr.app);
  return 0;
}

/* "/p 123", "/p:123" */
static HWND arg_hwnd(const char *p) {
  while (*p == ':' || *p == ' ') p++;
  if (!isdigit((unsigned char)*p)) return NULL;
  return (HWND)(uintptr_t)strtoull(p, NULL, 10);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
  (void)prev;
  (void)show;
  const char *p = cmd;
  while (*p == ' ') p++;
  if (*p == '\0') {
    scr_config_dialog(inst, NULL);
    return 0;
  }
  if (*p != '/' && *p != '-') return 1;
  char mode = (char)tolower((unsigned char)p[1]);
  HWND parent = p[1] != '\0' ? arg_hwnd(p + 2) : NULL;
  if (mode == 's') return run(inst, NULL);
  if (mode == 'p') {
    if (parent == NULL || !IsWindow(parent)) return 1;
    return run(inst, parent);
  }
  if (mode == 'c') scr_config_dialog(inst, parent != NULL ? parent : GetForegroundWindow());
  return 0;
}
