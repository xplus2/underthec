#include "color.h"
#include "rng.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

static int color_checked = 0;
static bool color_ok = false;

bool color_supported(void) {
  if (color_checked) return color_ok;
  color_checked = 1;
  if (!isatty(STDOUT_FILENO)) {
    color_ok = false;
    return color_ok;
  }
  if (getenv("NO_COLOR") != NULL) {
    color_ok = false;
    return color_ok;
  }
  const char *term = getenv("TERM");
  if (term != NULL && strcmp(term, "dumb") == 0) {
    color_ok = false;
    return color_ok;
  }
  color_ok = true;
  return color_ok;
}

struct attr color_from_mask_letter(char c) {
  struct attr a = {COL_DEFAULT, false};
  switch (c) {
  case 'c':
    a.col = COL_CYAN;
    a.bold = false;
    break;
  case 'C':
    a.col = COL_CYAN;
    a.bold = true;
    break;
  case 'r':
    a.col = COL_RED;
    a.bold = false;
    break;
  case 'R':
    a.col = COL_RED;
    a.bold = true;
    break;
  case 'y':
    a.col = COL_YELLOW;
    a.bold = false;
    break;
  case 'Y':
    a.col = COL_YELLOW;
    a.bold = true;
    break;
  case 'b':
    a.col = COL_BLUE;
    a.bold = false;
    break;
  case 'B':
    a.col = COL_BLUE;
    a.bold = true;
    break;
  case 'g':
    a.col = COL_GREEN;
    a.bold = false;
    break;
  case 'G':
    a.col = COL_GREEN;
    a.bold = true;
    break;
  case 'm':
    a.col = COL_MAGENTA;
    a.bold = false;
    break;
  case 'M':
    a.col = COL_MAGENTA;
    a.bold = true;
    break;
  case 'w':
    a.col = COL_WHITE;
    a.bold = false;
    break;
  case 'W':
    a.col = COL_WHITE;
    a.bold = true;
    break;
  case 'k':
    a.col = COL_BLACK;
    a.bold = false;
    break;
  case 'K':
    a.col = COL_BLACK;
    a.bold = true;
    break;
  default:
    a.col = COL_DEFAULT;
    a.bold = false;
    break;
  }
  return a;
}

struct attr color_from_name(const char *name) {
  struct attr a = {COL_DEFAULT, false};
  if (name == NULL || name[0] == '\0') return a;
  bool bold = (name[0] >= 'A' && name[0] <= 'Z');
  if (strcasecmp(name, "black") == 0)
    a.col = COL_BLACK;
  else if (strcasecmp(name, "red") == 0)
    a.col = COL_RED;
  else if (strcasecmp(name, "green") == 0)
    a.col = COL_GREEN;
  else if (strcasecmp(name, "yellow") == 0)
    a.col = COL_YELLOW;
  else if (strcasecmp(name, "blue") == 0)
    a.col = COL_BLUE;
  else if (strcasecmp(name, "magenta") == 0)
    a.col = COL_MAGENTA;
  else if (strcasecmp(name, "cyan") == 0)
    a.col = COL_CYAN;
  else if (strcasecmp(name, "white") == 0)
    a.col = COL_WHITE;
  else
    return (struct attr){COL_DEFAULT, false};

  a.bold = bold;
  return a;
}

void color_randomize_mask(const char *in, char *out) {
  static const char letters[] = {'c','C','r','R','y','Y','b','B','g','G','m','M'};
  char pick[10];
  for (int digit = 1; digit <= 9; digit++) pick[digit] = letters[rng_int((int)(sizeof(letters) / sizeof(letters[0])))];
  size_t len = strlen(in);
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c >= '1' && c <= '9') out[i] = pick[c - '0'];
    else out[i] = in[i];
  }
  out[len] = '\0';
}
