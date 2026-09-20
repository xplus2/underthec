#include "teletext.h"

#include <string.h>

#define MASK_TOP 1
#define MASK_MID 2
#define MASK_BOT 4
#define MASK_ALL 7

#define GFX_BASE 0x10
#define ALPHA_WHITE 0x07
#define MOSAIC_BLANK 0x20

#define X26_ACTIVE_POS 4
#define X26_DISPLAY_ROW0 7
#define X26_G0_CHAR 16

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

struct tcell {
  uint8_t ch;
  uint8_t col;
};

static const char nat_from[] = "@[\\]`{|}~";
static const char nat_alike[] = "a(/)'(!)-";

static struct tcell text_cell(const struct canvas *c, int x, int y) {
  struct tcell t = {0, 0};
  if (x >= c->width || y >= c->height) return t;
  const struct cell *g = &c->cells[(size_t)y * (size_t)c->width + (size_t)x];
  unsigned char ch = (unsigned char)g->glyph[0];
  uint8_t col = tt_color(g->col, g->bold);
  if (col == 0 || g->cont || ch == '\0' || ch == ' ') return t;
  t.ch = (ch < 0x21 || ch > 0x7E) ? '?' : ch;
  t.col = col;
  return t;
}

static void add_triplet(struct tt_page *p, uint8_t addr, uint8_t mode, uint8_t data) {
  p->ov[p->ov_count++] = (struct tt_triplet){addr, mode, data};
}

static uint8_t put_char(struct tt_page *p, const uint8_t allow[TT_ROWS][TT_COLS], int *ov_row, int y, int col, uint8_t ch) {
  const char *n = strchr(nat_from, ch);
  if (n == NULL) return ch;
  uint8_t alike = (uint8_t)nat_alike[n - nat_from];
  if (!allow[y][col]) return alike;
  if (*ov_row != y) {
    if (y == 0) add_triplet(p, 63, X26_DISPLAY_ROW0, 0);
    else add_triplet(p, (uint8_t)(y == 24 ? 40 : 40 + y), X26_ACTIVE_POS, 0);
    *ov_row = y;
  }
  add_triplet(p, (uint8_t)col, X26_G0_CHAR, ch == '@' ? 0x2A : ch);
  return alike;
}

static const char nat_priority[] = "\\|[]{}`~@";

static void plan_overlays(const struct canvas *c, uint8_t allow[TT_ROWS][TT_COLS]) {
  int used = 0;
  uint8_t row_used[TT_ROWS] = {0};
  memset(allow, 0, (size_t)TT_ROWS * TT_COLS);
  for (const char *k = nat_priority; *k != '\0'; k++) {
    int cells = 0;
    int rows = 0;
    uint8_t row_seen[TT_ROWS] = {0};
    for (int y = 0; y < TT_ROWS; y++) {
      for (int x = y == 0 ? TT_HDR_COL : 0; x < TT_COLS - 1; x++) {
        struct tcell t = text_cell(c, x, y);
        if (t.col == 0 || t.ch != (uint8_t)*k) continue;
        cells++;
        if (!row_used[y] && !row_seen[y]) {
          row_seen[y] = 1;
          rows++;
        }
      }
    }
    if (cells == 0 || used + cells + rows > TT_MAX_TRIPLETS) continue;
    for (int y = 0; y < TT_ROWS; y++) {
      for (int x = y == 0 ? TT_HDR_COL : 0; x < TT_COLS - 1; x++) {
        struct tcell t = text_cell(c, x, y);
        if (t.col != 0 && t.ch == (uint8_t)*k) allow[y][x + 1] = 1;
      }
      if (row_seen[y]) row_used[y] = 1;
    }
    used += cells + rows;
  }
}

static int glyph_weight(uint8_t ch) {
  return popcount3(glyph_bands[ch - 0x20]);
}

static void render_text_row(const struct canvas *c, const uint8_t allow[TT_ROWS][TT_COLS], int y, int first, int *ov_row, struct tt_page *p) {
  uint8_t *out = p->row[y];
  struct tcell cells[TT_COLS - 1];
  memset(out, MOSAIC_BLANK, TT_COLS);
  for (int i = 0; i < TT_COLS - 1; i++) cells[i] = i < first ? (struct tcell){0, 0} : text_cell(c, i, y);
  for (int i = 0; i < TT_COLS - 1;) {
    if (cells[i].col == 0) {
      i++;
      continue;
    }
    int weight[8] = {0};
    int j = i;
    for (; j < TT_COLS - 1 && cells[j].col != 0; j++) weight[cells[j].col] += glyph_weight(cells[j].ch);
    uint8_t col = 1;
    for (uint8_t k = 2; k <= 7; k++) if (weight[k] > weight[col]) col = k;
    out[i] = col;
    for (int k = i; k < j; k++) out[k + 1] = put_char(p, allow, ov_row, y, k + 1, cells[k].ch);
    i = j;
  }
}

static void overlay_title(uint8_t row[TT_COLS]) {
  const char title[] = TT_TITLE;
  bool tinted = false;
  for (int c = TT_HDR_COL; c < TT_COLS; c++) {
    uint8_t b = row[c];
    if (b >= 0x01 && b <= ALPHA_WHITE) tinted = b != ALPHA_WHITE;
    else if (b > GFX_BASE && b <= GFX_BASE + 7) tinted = true;
    int t = c - TT_HDR_COL;
    if (t >= (int)sizeof(title) - 1 || b != MOSAIC_BLANK) continue;
    if (tinted) {
      row[c] = ALPHA_WHITE;
      tinted = false;
    } else {
      row[c] = (uint8_t)title[t];
    }
  }
}

int tt_canvas_w(enum tt_glyphs g) {
  return g == TT_TEXT ? TT_COLS - 1 : (TT_COLS - 1) * 2;
}

void tt_render(const struct canvas *c, enum tt_glyphs g, struct tt_page *p) {
  int ov_row = -1;
  uint8_t allow[TT_ROWS][TT_COLS];
  p->ov_count = 0;
  if (g == TT_TEXT) plan_overlays(c, allow);
  for (int y = 0; y < TT_CANVAS_H; y++) {
    int first = y == 0 ? TT_HDR_COL : 0;
    if (g == TT_TEXT) render_text_row(c, allow, y, first, &ov_row, p);
    else render_row(c, y, first, p->row[y]);
  }
  overlay_title(p->row[0]);
}
