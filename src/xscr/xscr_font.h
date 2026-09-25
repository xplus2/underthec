#ifndef UNDERTHEC_XSCR_FONT_H
#define UNDERTHEC_XSCR_FONT_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stddef.h>

struct xscr_font;

/* wa.visual/wa.colormap of the target window, not the display default */
struct xscr_font *xscr_font_create(Display *dpy, int screen, Visual *visual, Colormap cmap, int font_px, char *err, size_t err_len);
void xscr_font_destroy(struct xscr_font *f);

int xscr_font_cell_w(const struct xscr_font *f);
int xscr_font_cell_h(const struct xscr_font *f);
int xscr_font_ascent(const struct xscr_font *f);

/* call after the backbuffer pixmap is (re)created */
void xscr_font_retarget(struct xscr_font *f, Drawable d);

void xscr_font_draw_run(struct xscr_font *f, Drawable d, GC gc, int x, int y, const char *utf8, int nbytes, int col, bool bold);

unsigned long xscr_font_pixel(const struct xscr_font *f, int col, bool bold);

#endif
