#ifndef UNDERTHEC_HELP_H
#define UNDERTHEC_HELP_H

#include <stdbool.h>

#include "canvas.h"

struct help_ui {
  bool open;
  const int *fps;
  const double *pace;
};

void help_ui_init(struct help_ui *ui, const int *fps, const double *pace);
bool help_ui_is_open(const struct help_ui *ui);
void help_ui_toggle(struct help_ui *ui);
void help_ui_close(struct help_ui *ui);
void help_ui_draw(const struct help_ui *ui, struct canvas *c);

#endif
