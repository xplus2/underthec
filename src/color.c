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
  static const char letters[] = "cCrRyYbBgGmMwWkK";
  static const enum color cols[8] = {COL_CYAN, COL_RED, COL_YELLOW, COL_BLUE, COL_GREEN, COL_MAGENTA, COL_WHITE, COL_BLACK};
  if (c == '\0') return (struct attr){COL_DEFAULT, false};
  const char *p = strchr(letters, c);
  if (p == NULL) return (struct attr){COL_DEFAULT, false};
  int idx = (int)(p - letters);
  return (struct attr){cols[idx / 2], (idx % 2) == 1};
}

struct attr color_from_name(const char *name) {
  static const char *const names[8] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
  static const enum color cols[8] = {COL_BLACK, COL_RED, COL_GREEN, COL_YELLOW, COL_BLUE, COL_MAGENTA, COL_CYAN, COL_WHITE};
  if (name == NULL || name[0] == '\0') return (struct attr){COL_DEFAULT, false};
  bool bold = (name[0] >= 'A' && name[0] <= 'Z');
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    if (strcasecmp(name, names[i]) == 0) return (struct attr){cols[i], bold};
  }
  return (struct attr){COL_DEFAULT, false};
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
