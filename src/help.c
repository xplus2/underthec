#include "help.h"
#include "color.h"
#include "version.h"

#include <stdio.h>

#define CONTENT_W 20
#define CONTENT_H 10
#define MARGIN 1
#define BOX_W (CONTENT_W + 2 * MARGIN)
#define BOX_H (CONTENT_H + 2 * MARGIN)

static const struct attr BOX_ATTR = {.col = COL_WHITE, .bold = false, .bg = COL_BLACK, .bg_bold = true};
static const struct attr TITLE_ATTR = {.col = COL_WHITE, .bold = true, .bg = COL_BLACK, .bg_bold = true};

void help_ui_init(struct help_ui *ui, const int *fps, const double *pace) {
  ui->open = false;
  ui->fps = fps;
  ui->pace = pace;
}

bool help_ui_is_open(const struct help_ui *ui) { return ui->open; }

void help_ui_toggle(struct help_ui *ui) { ui->open = !ui->open; }

void help_ui_close(struct help_ui *ui) { ui->open = false; }

static void fill_rect(struct canvas *c, int x0, int y0, int w, int h, struct attr a) {
  for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) canvas_put(c, x0 + x, y0 + y, " ", 1, a, 1);
}

static void draw_row_text(struct canvas *c, int x0, int y, const char *s, struct attr a) {
  for (int i = 0; s[i] != '\0'; i++) {
    char g[2] = {s[i], '\0'};
    canvas_put(c, x0 + i, y, g, 1, a, 1);
  }
}

void help_ui_draw(const struct help_ui *ui, struct canvas *c) {
  if (!ui->open) return;

  fill_rect(c, 0, 0, BOX_W, BOX_H, BOX_ATTR);

  char line[CONTENT_W + 1];
  int y = MARGIN;
  snprintf(line, sizeof line, TOOL_DISPLAY_NAME " v%s", TOOL_VERSION);
  draw_row_text(c, MARGIN, y++, line, TITLE_ATTR);
  y++;
  draw_row_text(c, MARGIN, y++, "keys:", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  f   feed the fish", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  h   toggle help", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  p   pause/resume", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  q   quit", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  r   redraw", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  s   settings", BOX_ATTR);
  draw_row_text(c, MARGIN, y++, "  t   transparency", BOX_ATTR);
  y++;
}
