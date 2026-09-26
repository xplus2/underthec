#include "xscr_font.h"

#include "../../xalloc.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#else
#include <X11/Xlib.h>
#endif

/* [enum color][bold] */
static const unsigned char palette_rgb[9][2][3] = {
{{0xc0, 0xc0, 0xc0}, {0xff, 0xff, 0xff}},
{{0x00, 0x00, 0x00}, {0x80, 0x80, 0x80}},
{{0xc0, 0x00, 0x00}, {0xff, 0x55, 0x55}},
{{0x00, 0xc0, 0x00}, {0x55, 0xff, 0x55}},
{{0xc0, 0xc0, 0x00}, {0xff, 0xff, 0x55}},
{{0x00, 0x00, 0xc0}, {0x55, 0x55, 0xff}},
{{0xc0, 0x00, 0xc0}, {0xff, 0x55, 0xff}},
{{0x00, 0xc0, 0xc0}, {0x55, 0xff, 0xff}},
{{0xc0, 0xc0, 0xc0}, {0xff, 0xff, 0xff}},
};

struct xscr_font {
  Display *dpy;
  Visual *visual;
  Colormap cmap;
  unsigned long pixel[9][2];
  int cell_w;
  int cell_h;
  int ascent;
#ifdef HAVE_XFT
  XftFont *font[2];
  XftDraw *draw;
  XftColor xcolor[9][2];
#else
  XFontSet fontset[2];
  XFontStruct *basic;
#endif
};

#ifdef HAVE_XFT

static bool fonts_load(struct xscr_font *f, int screen, int font_px, char *err, size_t err_len) {
  char name[64];
  for (int b = 0; b < 2; b++) {
    snprintf(name, sizeof name, "monospace:pixelsize=%d%s", font_px, b ? ":style=Bold" : "");
    f->font[b] = XftFontOpenName(f->dpy, screen, name);
    if (f->font[b] == NULL) {
      snprintf(err, err_len, "XftFontOpenName failed for '%s'", name);
      return false;
    }
  }
  f->cell_w = f->font[0]->max_advance_width;
  f->cell_h = f->font[0]->ascent + f->font[0]->descent;
  f->ascent = f->font[0]->ascent;
  return true;
}

static void fonts_free(struct xscr_font *f) {
  for (int b = 0; b < 2; b++) {
    if (f->font[b] != NULL) XftFontClose(f->dpy, f->font[b]);
  }
}

static void palette_load(struct xscr_font *f) {
  for (int c = 0; c < 9; c++) {
    for (int b = 0; b < 2; b++) {
      XRenderColor rc = {
          .red = (unsigned short)(palette_rgb[c][b][0] * 0x101),
          .green = (unsigned short)(palette_rgb[c][b][1] * 0x101),
          .blue = (unsigned short)(palette_rgb[c][b][2] * 0x101),
          .alpha = 0xffff,
      };
      XftColorAllocValue(f->dpy, f->visual, f->cmap, &rc, &f->xcolor[c][b]);
      f->pixel[c][b] = f->xcolor[c][b].pixel;
    }
  }
}

static void palette_free(struct xscr_font *f) {
  for (int c = 0; c < 9; c++)
    for (int b = 0; b < 2; b++) XftColorFree(f->dpy, f->visual, f->cmap, &f->xcolor[c][b]);
}

void xscr_font_retarget(struct xscr_font *f, Drawable d) {
  if (f->draw == NULL) f->draw = XftDrawCreate(f->dpy, d, f->visual, f->cmap);
  else XftDrawChange(f->draw, d);
}

void xscr_font_draw_run(struct xscr_font *f, Drawable d, GC gc, int x, int y, const char *utf8, int nbytes, int col, bool bold) {
  (void)d;
  (void)gc;
  XftDrawStringUtf8(f->draw, &f->xcolor[col][bold ? 1 : 0], f->font[bold ? 1 : 0], x, y, (const FcChar8 *)utf8, nbytes);
}

#else

static bool fontset_try(struct xscr_font *f, int b, const char *base, char ***missing, int *nmissing, char **def) {
  f->fontset[b] = XCreateFontSet(f->dpy, base, missing, nmissing, def);
  if (*nmissing > 0 && *missing != NULL) XFreeStringList(*missing);
  return f->fontset[b] != NULL;
}

static bool fonts_load(struct xscr_font *f, int screen, int font_px, char *err, size_t err_len) {
  (void)screen;
  (void)err;
  (void)err_len;
  setlocale(LC_CTYPE, "");
  char normal[256];
  char bold[256];
  snprintf(normal, sizeof normal, "-*-*-medium-r-normal--%d-*-*-*-*-*-iso10646-1,-*-*-medium-r-normal--%d-*-*-*-*-*-iso8859-1,fixed", font_px, font_px);
  snprintf(bold, sizeof bold, "-*-*-bold-r-normal--%d-*-*-*-*-*-iso10646-1,-*-*-bold-r-normal--%d-*-*-*-*-*-iso8859-1,fixed", font_px, font_px);
  char **missing;
  int nmissing;
  char *def;
  bool ok_n = fontset_try(f, 0, normal, &missing, &nmissing, &def);
  bool ok_b = fontset_try(f, 1, bold, &missing, &nmissing, &def);
  if (!ok_n || !ok_b) {
    if (f->fontset[0] != NULL) XFreeFontSet(f->dpy, f->fontset[0]);
    if (f->fontset[1] != NULL) XFreeFontSet(f->dpy, f->fontset[1]);
    f->fontset[0] = f->fontset[1] = NULL;
    f->basic = XLoadQueryFont(f->dpy, "fixed");
    if (f->basic == NULL) return false;
    f->cell_w = XTextWidth(f->basic, "M", 1);
    f->cell_h = f->basic->ascent + f->basic->descent;
    f->ascent = f->basic->ascent;
    return true;
  }
  XRectangle ink;
  XRectangle logical;
  Xutf8TextExtents(f->fontset[0], "M", 1, &ink, &logical);
  f->cell_w = Xutf8TextEscapement(f->fontset[0], "M", 1);
  f->cell_h = logical.height;
  f->ascent = -logical.y;
  return true;
}

static void fonts_free(struct xscr_font *f) {
  if (f->fontset[0] != NULL) XFreeFontSet(f->dpy, f->fontset[0]);
  if (f->fontset[1] != NULL) XFreeFontSet(f->dpy, f->fontset[1]);
  if (f->basic != NULL) XFreeFont(f->dpy, f->basic);
}

static void palette_load(struct xscr_font *f) {
  for (int c = 0; c < 9; c++) {
    for (int b = 0; b < 2; b++) {
      XColor xc = {
          .red = (unsigned short)(palette_rgb[c][b][0] * 0x101),
          .green = (unsigned short)(palette_rgb[c][b][1] * 0x101),
          .blue = (unsigned short)(palette_rgb[c][b][2] * 0x101),
          .flags = DoRed | DoGreen | DoBlue,
      };
      if (XAllocColor(f->dpy, f->cmap, &xc)) f->pixel[c][b] = xc.pixel;
      else f->pixel[c][b] = b ? WhitePixel(f->dpy, DefaultScreen(f->dpy)) : BlackPixel(f->dpy, DefaultScreen(f->dpy));
    }
  }
}

static void palette_free(struct xscr_font *f) {
  unsigned long px[18];
  int n = 0;
  for (int c = 0; c < 9; c++) for (int b = 0; b < 2; b++) px[n++] = f->pixel[c][b];
  XFreeColors(f->dpy, f->cmap, px, n, 0);
}

void xscr_font_retarget(struct xscr_font *f, Drawable d) {
  (void)f;
  (void)d; /* core-font drawing takes the drawable per call, nothing to rebind */
}

void xscr_font_draw_run(struct xscr_font *f, Drawable d, GC gc, int x, int y, const char *utf8, int nbytes, int col, bool bold) {
  XSetForeground(f->dpy, gc, f->pixel[col][bold ? 1 : 0]);
  if (f->basic != NULL) {
    XSetFont(f->dpy, gc, f->basic->fid);
    XDrawString(f->dpy, d, gc, x, y, utf8, nbytes);
  } else {
    Xutf8DrawString(f->dpy, d, f->fontset[bold ? 1 : 0], gc, x, y, utf8, nbytes);
  }
}

#endif

struct xscr_font *xscr_font_create(Display *dpy, int screen, Visual *visual, Colormap cmap, int font_px, char *err, size_t err_len) {
  struct xscr_font *f = xcalloc(1, sizeof *f);
  f->dpy = dpy;
  f->visual = visual;
  f->cmap = cmap;
  if (!fonts_load(f, screen, font_px, err, err_len)) {
    free(f);
    return NULL;
  }
  palette_load(f);
  return f;
}

void xscr_font_destroy(struct xscr_font *f) {
  if (f == NULL) return;
  palette_free(f);
  fonts_free(f);
#ifdef HAVE_XFT
  if (f->draw != NULL) XftDrawDestroy(f->draw);
#endif
  free(f);
}

int xscr_font_cell_w(const struct xscr_font *f) { return f->cell_w; }
int xscr_font_cell_h(const struct xscr_font *f) { return f->cell_h; }
int xscr_font_ascent(const struct xscr_font *f) { return f->ascent; }

unsigned long xscr_font_pixel(const struct xscr_font *f, int col, bool bold) { return f->pixel[col][bold ? 1 : 0]; }
