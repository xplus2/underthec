#include "../app.h"
#include "../config.h"
#include "xscr_font.h"

#include <X11/Xlib.h>

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

#define RUN_MAX 512

struct xscr {
  struct app app;
  bool started;
  Display *dpy;
  Window win;
  int screen;
  struct xscr_font *font;
  int cell_w;
  int cell_h;
  int off_x;
  int off_y;
  Pixmap backbuf;
  GC gc;
  int bmp_w;
  int bmp_h;
};

static volatile sig_atomic_t g_should_quit = 0;

static void on_signal(int sig) {
  (void)sig;
  g_should_quit = 1;
}

static double now_seconds(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static bool arg_window_id(const char *s, Window *out) {
  if (s[0] == '\0') return false;
  char *end = NULL;
  unsigned long v = strtoul(s, &end, 0);
  if (*end != '\0') return false;
  *out = (Window)v;
  return true;
}

static void backbuf_free(struct xscr *x) {
  if (x->backbuf == None) return;
  XFreePixmap(x->dpy, x->backbuf);
  x->backbuf = None;
}

static void layout(struct xscr *x, int w, int h) {
  if (w <= 0 || h <= 0) return;
  XWindowAttributes wa;
  XGetWindowAttributes(x->dpy, x->win, &wa);
  backbuf_free(x);
  x->backbuf = XCreatePixmap(x->dpy, x->win, (unsigned)w, (unsigned)h, (unsigned)wa.depth);
  xscr_font_retarget(x->font, x->backbuf);
  x->bmp_w = w;
  x->bmp_h = h;
  int cols = w / x->cell_w;
  int rows = h / x->cell_h;
  if (cols < 1) cols = 1;
  if (rows < 1) rows = 1;
  x->off_x = (w - cols * x->cell_w) / 2;
  x->off_y = (h - rows * x->cell_h) / 2;
  if (x->started) app_resize(&x->app, cols, rows);
}

/* runs of same fg attr per row, font's own advance (monospace ASCII content) */
static void draw_row(struct xscr *x, int row) {
  const struct canvas *c = &x->app.canvas;
  const struct cell *line = &c->cells[(size_t)row * (size_t)c->width];
  int y = x->off_y + row * x->cell_h + xscr_font_ascent(x->font);
  char text[RUN_MAX];
  int n = 0;
  int run_x = x->off_x;
  const struct cell *run_attr = NULL;
  for (int col = 0; col < c->width; col++) {
    const struct cell *cell = &line[col];
    if (cell->cont) continue;
    bool same = run_attr != NULL && run_attr->col == cell->col && run_attr->bold == cell->bold;
    size_t len = strlen(cell->glyph);
    if (!same || n + (int)len > RUN_MAX) {
      if (run_attr != NULL) xscr_font_draw_run(x->font, x->backbuf, x->gc, run_x, y, text, n, run_attr->col, run_attr->bold);
      n = 0;
      run_x = x->off_x + col * x->cell_w;
      run_attr = cell;
    }
    memcpy(text + n, cell->glyph, len);
    n += (int)len;
  }
  if (run_attr != NULL) xscr_font_draw_run(x->font, x->backbuf, x->gc, run_x, y, text, n, run_attr->col, run_attr->bold);
}

static void draw_canvas(struct xscr *x) {
  const struct canvas *c = &x->app.canvas;
  for (int row = 0; row < c->height; row++) {
    for (int col = 0; col < c->width; col++) {
      const struct cell *cell = &c->cells[(size_t)row * (size_t)c->width + (size_t)col];
      if (cell->bg == COL_DEFAULT) continue;
      XSetForeground(x->dpy, x->gc, xscr_font_pixel(x->font, cell->bg, cell->bg_bold));
      XFillRectangle(x->dpy, x->backbuf, x->gc, x->off_x + col * x->cell_w, x->off_y + row * x->cell_h, (unsigned)x->cell_w, (unsigned)x->cell_h);
    }
  }
  for (int row = 0; row < c->height; row++) draw_row(x, row);
}

static void present(struct xscr *x) {
  if (x->backbuf == None) return;
  XSetForeground(x->dpy, x->gc, BlackPixel(x->dpy, x->screen));
  XFillRectangle(x->dpy, x->backbuf, x->gc, 0, 0, (unsigned)x->bmp_w, (unsigned)x->bmp_h);
  if (x->started) draw_canvas(x);
  XCopyArea(x->dpy, x->backbuf, x->win, x->gc, 0, 0, (unsigned)x->bmp_w, (unsigned)x->bmp_h, 0, 0);
  XFlush(x->dpy);
}

static void cursor_hide(Display *dpy, Window win) {
  char pixel[1] = {0};
  Pixmap blank = XCreateBitmapFromData(dpy, win, pixel, 1, 1);
  XColor dummy = {0};
  Cursor cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
  XDefineCursor(dpy, win, cursor);
  XFreeCursor(dpy, cursor);
  XFreePixmap(dpy, blank);
}

int main(int argc, char **argv) {
  Window target = None;
  bool have_window = false;
  struct config cfg;
  config_init(&cfg);
  char err[256] = {0};
  bool cfg_ok = true;

  int i = 1;
  while (i < argc) {
    const char *a = argv[i];
    if (a[0] != '-') {
      i++;
      continue;
    }
    const char *name = a;
    while (*name == '-') name++;
    if (strcmp(name, "window-id") == 0) {
      if (i + 1 >= argc || !arg_window_id(argv[i + 1], &target)) {
        fprintf(stderr, "%s: -window-id requires a numeric argument\n", argv[0]);
        config_free(&cfg);
        return 1;
      }
      have_window = true;
      i += 2;
      continue;
    }
    if (i + 1 >= argc) {
      fprintf(stderr, "%s: %s requires an argument\n", argv[0], a);
      config_free(&cfg);
      return 1;
    }
    if (!config_set(&cfg, name, argv[i + 1], name, err, sizeof err)) cfg_ok = false;
    i += 2;
  }
  if (!have_window) {
    fprintf(stderr, "%s: -window-id <id> required (run via xscreensaver, not directly)\n", argv[0]);
    config_free(&cfg);
    return 1;
  }
  if (cfg_ok) cfg_ok = config_check(&cfg, err, sizeof err);
  if (!cfg_ok) {
    fprintf(stderr, "%s: %s\n", argv[0], err);
    config_free(&cfg);
    return 1;
  }

  Display *dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "%s: cannot open X display\n", argv[0]);
    config_free(&cfg);
    return 1;
  }

  struct xscr x = {0};
  x.dpy = dpy;
  x.win = target;
  x.screen = DefaultScreen(dpy);

  XWindowAttributes wa;
  if (!XGetWindowAttributes(dpy, x.win, &wa)) {
    fprintf(stderr, "%s: invalid -window-id\n", argv[0]);
    config_free(&cfg);
    XCloseDisplay(dpy);
    return 1;
  }
  XSelectInput(dpy, x.win, ExposureMask | StructureNotifyMask);
  XSetWindowBackground(dpy, x.win, BlackPixel(dpy, x.screen));
  XClearWindow(dpy, x.win);
  cursor_hide(dpy, x.win);
  x.gc = XCreateGC(dpy, x.win, 0, NULL);

  int font_px = wa.height / 45;
  if (font_px < 10) font_px = 10;
  char ferr[256];
  x.font = xscr_font_create(dpy, x.screen, wa.visual, wa.colormap, font_px, ferr, sizeof ferr);
  if (x.font == NULL) {
    fprintf(stderr, "%s: %s\n", argv[0], ferr);
    config_free(&cfg);
    XFreeGC(dpy, x.gc);
    XCloseDisplay(dpy);
    return 1;
  }
  x.cell_w = xscr_font_cell_w(x.font);
  x.cell_h = xscr_font_cell_h(x.font);

  config_start(&cfg, &x.app, now_seconds());
  x.started = true;
  int fps = cfg.fps;
  config_free(&cfg);

  signal(SIGTERM, on_signal);
  signal(SIGINT, on_signal);

  layout(&x, wa.width, wa.height);

  int fd = ConnectionNumber(dpy);
  double frame_period = 1.0 / (double)fps;
  double deadline = now_seconds();
  while (!g_should_quit) {
    while (XPending(dpy) > 0) {
      XEvent ev;
      XNextEvent(dpy, &ev);
      if (ev.type == ConfigureNotify) layout(&x, ev.xconfigure.width, ev.xconfigure.height);
      else if (ev.type == Expose) present(&x);
      else if (ev.type == DestroyNotify) g_should_quit = 1;
    }
    double t = now_seconds();
    if (t >= deadline) {
      app_frame(&x.app, t);
      present(&x);
      deadline += frame_period;
      if (deadline < t) deadline = t + frame_period;
      continue;
    }
    double wait = deadline - t;
    struct timeval tv;
    tv.tv_sec = (time_t)wait;
    tv.tv_usec = (long)((wait - (double)tv.tv_sec) * 1e6);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    select(fd + 1, &rfds, NULL, NULL, &tv);
  }

  app_free(&x.app);
  xscr_font_destroy(x.font);
  backbuf_free(&x);
  XFreeGC(dpy, x.gc);
  XCloseDisplay(dpy);
  return 0;
}
