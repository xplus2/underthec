#include "priv.h"

#include <string.h>

static bool bbox_overlap(struct entity *a, struct entity *b) {
  int aw = entity_width(a);
  int ah = entity_height(a);
  int bw = entity_width(b);
  int bh = entity_height(b);
  return a->x < b->x + bw && b->x < a->x + aw && a->y < b->y + bh && b->y < a->y + ah;
}

void entity_collide_all(struct entity_list *list) {
  for (int i = 0; i < list->count; i++) {
    struct entity *fish = &list->items[i];
    if (fish->marked_dead || !fish->physical || fish->type != ENT_FISH) continue;
    if (entity_height(fish) > 5) continue;
    for (int j = 0; j < list->count; j++) {
      struct entity *teeth = &list->items[j];
      if (teeth->marked_dead || !teeth->physical || teeth->type != ENT_TEETH) continue;
      if (bbox_overlap(fish, teeth)) {
        fish->marked_dead = true;
        fish->spawn_splat = true;
        fish->splat_x = teeth->x;
        fish->splat_y = teeth->y;
        fish->splat_z = teeth->z;
        break;
      }
    }
  }
  for (int i = 0; i < list->count; i++) {
    struct entity *bubble = &list->items[i];
    if (bubble->marked_dead || !bubble->physical || bubble->type != ENT_BUBBLE) continue;
    for (int j = 0; j < list->count; j++) {
      struct entity *wl = &list->items[j];
      if (wl->marked_dead || !wl->physical || wl->type != ENT_WATERLINE) continue;
      if (bbox_overlap(bubble, wl)) {
        bubble->marked_dead = true;
        break;
      }
    }
  }
}

bool cell_transparent(const struct entity *e, const char *srow, int len, int col) {
  char ch = srow[col];
  if (e->sentinel != 0 && ch == e->sentinel) return true;
  if (e->trim_edges && ch == ' ') {
    int start = 0;
    while (start < len && srow[start] == ' ') start++;
    int end = len;
    while (end > start && srow[end - 1] == ' ') end--;
    if (col < start || col >= end) return true;
  }
  return false;
}

bool entity_glyph_overlap(struct entity *a, struct entity *b) {
  ascii_rows arows = entity_shape(a);
  ascii_rows brows = entity_shape(b);
  if (arows == NULL || brows == NULL) return false;

  int aw = entity_width(a);
  int ah = entity_height(a);
  int bw = entity_width(b);
  int bh = entity_height(b);
  int ax = round_to_int(a->x);
  int ay = round_to_int(a->y);
  int bx = round_to_int(b->x);
  int by = round_to_int(b->y);

  int x0 = ax > bx ? ax : bx;
  int x1 = (ax + aw) < (bx + bw) ? (ax + aw) : (bx + bw);
  int y0 = ay > by ? ay : by;
  int y1 = (ay + ah) < (by + bh) ? (ay + ah) : (by + bh);
  for (int wy = y0; wy < y1; wy++) {
    int arow = wy - ay;
    int brow = wy - by;
    if (arow < 0 || arow >= ah || brow < 0 || brow >= bh) continue;
    const char *asrow = arows[arow];
    const char *bsrow = brows[brow];
    if (asrow == NULL || bsrow == NULL) continue;
    int abytes = (int)strlen(asrow);
    int bbytes = (int)strlen(bsrow);
    int acols = utf8_col_width(asrow);
    int bcols = utf8_col_width(bsrow);
    for (int wx = x0; wx < x1; wx++) {
      int acol = wx - ax;
      int bcol = wx - bx;
      if (acol < 0 || acol >= acols || bcol < 0 || bcol >= bcols) continue;
      int aoff = utf8_byte_offset(asrow, acol);
      int boff = utf8_byte_offset(bsrow, bcol);
      if (cell_transparent(a, asrow, abytes, aoff)) continue;
      if (cell_transparent(b, bsrow, bbytes, boff)) continue;
      return true;
    }
  }
  return false;
}
