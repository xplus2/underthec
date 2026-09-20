#include "teletext.h"

#include <string.h>

#define MASK_TOP 1
#define MASK_MID 2
#define MASK_BOT 4
#define MASK_ALL 7

#define GFX_BASE 0x10
#define ALPHA_WHITE 0x07
#define MOSAIC_BLANK 0x20

struct mcell {
  uint8_t code;
  uint8_t col[2]; /* per glyph, 1..7, 0 = blank */
  int weight[2];
  bool ink;
};

/* top/mid/bottom ink for 0x20..0x7E, from monospace font cap-height thirds */
static const uint8_t glyph_bands[95] = {
  0, 7, 1, 7, 7, 7, 7, 1, 7, 7, 3, 6, 4, 2, 4, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 1, 4, 1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 7, 7, 7, 7, 2,
};

static int glyph_mask(const struct cell *c) {
  if (c->cont) return MASK_ALL;
  unsigned char g = (unsigned char)c->glyph[0];
  if (g == '\0') return 0;
  if (g < 0x20 || g > 0x7E) return MASK_ALL;
  return glyph_bands[g - 0x20];
}

static uint8_t tt_color(enum color c, bool bold) {
  switch (c) {
  case COL_RED: return 1;
  case COL_GREEN: return 2;
  case COL_YELLOW: return 3;
  case COL_BLUE: return 4;
  case COL_MAGENTA: return 5;
  case COL_CYAN: return 6;
  case COL_BLACK: return bold ? 7 : 0; /* bold black = grey, nearest is white */
  default: return 7;
  }
}

static int popcount3(int m) {
  return (m & 1) + ((m >> 1) & 1) + ((m >> 2) & 1);
}

static int cell_mask(const struct cell *c, uint8_t *col) {
  *col = tt_color(c->col, c->bold);
  return *col == 0 ? 0 : glyph_mask(c);
}

static struct mcell mosaic_cell(const struct canvas *c, int x, int y) {
  struct cell blank = {{' ', '\0'}, COL_DEFAULT, false, false};
  const struct cell *g[2] = {&blank, &blank};
  if (y < c->height) for (int k = 0; k < 2; k++) if (x + k < c->width) g[k] = &c->cells[(size_t)y * (size_t)c->width + (size_t)(x + k)];
  int mask[2];
  struct mcell m;
  for (int k = 0; k < 2; k++) {
    mask[k] = cell_mask(g[k], &m.col[k]);
    m.weight[k] = popcount3(mask[k]);
  }
  m.ink = m.weight[0] + m.weight[1] > 0;
  /* sixel bits: TL 1, TR 2, ML 4, MR 8, BL 16, BR 64 */
  m.code = (uint8_t)(MOSAIC_BLANK | (mask[0] & MASK_TOP) | ((mask[1] & MASK_TOP) << 1) | ((mask[0] & MASK_MID) << 1) | ((mask[1] & MASK_MID) << 2) | ((mask[0] & MASK_BOT) << 2) | ((mask[1] & MASK_BOT) << 4));
  return m;
}

static void render_row(const struct canvas *c, int y, int first, uint8_t out[TT_COLS]) {
  struct mcell cells[TT_COLS - 1];
  memset(out, MOSAIC_BLANK, TT_COLS);
  for (int i = 0; i < TT_COLS - 1; i++) cells[i] = mosaic_cell(c, i * 2, y);
  for (int i = 0; i < first; i++) cells[i].ink = false;
  /* run of ink gets one color, control goes in blank cell before it */
  for (int i = 0; i < TT_COLS - 1;) {
    if (!cells[i].ink) {
      i++;
      continue;
    }
    int weight[8] = {0};
    int j = i;
    for (; j < TT_COLS - 1 && cells[j].ink; j++) for (int k = 0; k < 2; k++) weight[cells[j].col[k]] += cells[j].weight[k];
    uint8_t col = 1;
    for (uint8_t k = 2; k <= 7; k++) if (weight[k] > weight[col]) col = k;
    out[i] = (uint8_t)(GFX_BASE | col);
    for (int k = i; k < j; k++) out[k + 1] = cells[k].code;
    i = j;
  }
}

static void overlay_title(uint8_t row[TT_COLS]) {
  const char title[] = TT_TITLE;
  bool gfx = false;
  for (int c = TT_HDR_COL; c < TT_COLS; c++) {
    if (row[c] > GFX_BASE && row[c] <= GFX_BASE + 7) gfx = true;
    int t = c - TT_HDR_COL;
    if (t >= (int)sizeof(title) - 1 || row[c] != MOSAIC_BLANK) continue;
    if (gfx) {
      row[c] = ALPHA_WHITE;
      gfx = false;
    } else {
      row[c] = (uint8_t)title[t];
    }
  }
}

void tt_render(const struct canvas *c, struct tt_page *p) {
  for (int y = 0; y < TT_CANVAS_H; y++) render_row(c, y, y == 0 ? TT_HDR_COL : 0, p->row[y]);
  overlay_title(p->row[0]);
}
