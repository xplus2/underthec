#ifndef UNDERTHEC_ART_ROWERS_H
#define UNDERTHEC_ART_ROWERS_H

#include "../sprite.h"

static const char *const rowers_image_0_0[] = {
  "  q  q  q  q",
  "\\==\\==\\==\\==\\==/",
  "    \\  \\  \\  \\",
  NULL
};

static const char *const rowers_image_0_1[] = {
  "   o  o  o  o",
  "\\==|==|==|==|==/",
  "   |  |  |  |",
  NULL
};

static const char *const rowers_image_0_2[] = {
  "    p  p  p  p",
  "\\==/==/==/==/==/",
  "  /  /  /  /",
  NULL
};

static const char *const rowers_image_0_3[] = {
  "___o__o__o__o",
  "\\==============/",
  NULL
};

static const char *const rowers_image_0_4[] = {
  "  q__q__q__q___",
  "\\==============/",
  NULL
};

static const char *const rowers_image_1_0[] = {
  "    p  p  p  p  ",
  "\\==/==/==/==/==/",
  "  /  /  /  /    ",
  NULL
};

static const char *const rowers_image_1_1[] = {
  "   o  o  o  o   ",
  "\\==|==|==|==|==/",
  "   |  |  |  |   ",
  NULL
};

static const char *const rowers_image_1_2[] = {
  "  q  q  q  q    ",
  "\\==\\==\\==\\==\\==/",
  "    \\  \\  \\  \\  ",
  NULL
};

static const char *const rowers_image_1_3[] = {
  "   o__o__o__o___",
  "\\==============/",
  NULL
};

static const char *const rowers_image_1_4[] = {
  " ___p__p__p__p  ",
  "\\==============/",
  NULL
};

static const char *const rowers_mask_0_0[] = {
  "",
  "rrr rr rr rr rrr",
  "",
  NULL
};

static const char *const rowers_mask_0_1[] = {
  "",
  "rrr rr rr rr rrr",
  "",
  NULL
};

static const char *const rowers_mask_0_2[] = {
  "",
  "rrr rr rr rr rrr",
  "",
  NULL
};

static const char *const rowers_mask_0_3[] = {
  "",
  "rrrrrrrrrrrrrrrr",
  NULL
};

static const char *const rowers_mask_0_4[] = {
  "",
  "rrrrrrrrrrrrrrrr",
  NULL
};

static const char *const rowers_mask_1_0[] = {
  "",
  "rrr rr rr rr rrr",
  "",
  NULL
};

static const char *const rowers_mask_1_1[] = {
  "",
  "rrr rr rr rr rrr",
  "",
  NULL
};

static const char *const rowers_mask_1_2[] = {
  "",
  "rrr rr rr rr rrr",
  "",
  NULL
};

static const char *const rowers_mask_1_3[] = {
  "",
  "rrrrrrrrrrrrrrrr",
  NULL
};

static const char *const rowers_mask_1_4[] = {
  "",
  "rrrrrrrrrrrrrrrr",
  NULL
};

static const struct sprite_pair rowers[2][5] = {
  {
    {rowers_image_0_0, rowers_mask_0_0},
    {rowers_image_0_1, rowers_mask_0_1},
    {rowers_image_0_2, rowers_mask_0_2},
    {rowers_image_0_3, rowers_mask_0_3},
    {rowers_image_0_4, rowers_mask_0_4},
  },
  {
    {rowers_image_1_0, rowers_mask_1_0},
    {rowers_image_1_1, rowers_mask_1_1},
    {rowers_image_1_2, rowers_mask_1_2},
    {rowers_image_1_3, rowers_mask_1_3},
    {rowers_image_1_4, rowers_mask_1_4},
  },
};

#endif
