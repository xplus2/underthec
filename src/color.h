#ifndef UNDERTHEC_COLOR_H
#define UNDERTHEC_COLOR_H

#include <stdbool.h>

enum color {
  COL_DEFAULT = 0,
  COL_BLACK,
  COL_RED,
  COL_GREEN,
  COL_YELLOW,
  COL_BLUE,
  COL_MAGENTA,
  COL_CYAN,
  COL_WHITE
};

struct attr {
  enum color col;
  bool bold;
};

/* detect tty color support */
bool color_supported(void);

/* mask letters */
struct attr color_from_mask_letter(char c);
struct attr color_from_name(const char *name);
void color_randomize_mask(const char *in, char *out);

#endif
