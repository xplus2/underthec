#ifndef UNDERTHEC_SPRITE_H
#define UNDERTHEC_SPRITE_H

#include <stddef.h>

typedef const char *const *ascii_rows;

struct sprite_pair {
  ascii_rows shape;
  ascii_rows mask;
};

#endif
