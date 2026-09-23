#ifndef UNDERTHEC_SETTINGS_H
#define UNDERTHEC_SETTINGS_H

#include <stdbool.h>

#include "canvas.h"
#include "scene.h"

struct settings_ui {
  bool open;
  int *fps;
  double *pace;
  struct scene *scene;
  int sel_row;
  int sel_col;
};

void settings_ui_init(struct settings_ui *ui, int *fps, double *pace, struct scene *scene);
bool settings_ui_is_open(const struct settings_ui *ui);
void settings_ui_toggle(struct settings_ui *ui);
void settings_ui_close(struct settings_ui *ui);
void settings_ui_handle_key(struct settings_ui *ui, int key, int term_w, int term_h);
/* x,y=cells. true=in box */
bool settings_ui_click(struct settings_ui *ui, int x, int y, int term_w, int term_h);
void settings_ui_draw(const struct settings_ui *ui, struct canvas *c);

#endif
