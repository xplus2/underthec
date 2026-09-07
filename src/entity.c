#include "entity.h"
#include "color.h"

#include <stdlib.h>
#include <string.h>

static int round_to_int(double v) {
  return (int)(v >= 0.0 ? v + 0.5 : v - 0.5);
}

static void free_row_array(char **rows) {
  if (rows == NULL) return;
  for (int i = 0; rows[i] != NULL; i++) free(rows[i]);
  free(rows);
}

void entity_clear_owned(struct entity *e) {
  if (e->owned_mask != NULL) {
    free_row_array(e->owned_mask);
    e->owned_mask = NULL;
  }
  if (e->owned_frame_table != NULL) {
    for (int i = 0; i < e->frame_count; i++) free_row_array(e->owned_shape_rows[i]);
    free(e->owned_shape_rows);
    e->owned_shape_rows = NULL;
    free(e->owned_frame_table);
    e->owned_frame_table = NULL;
  }
}

void entity_list_init(struct entity_list *list) {
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

void entity_list_clear(struct entity_list *list) {
  for (int i = 0; i < list->count; i++) entity_clear_owned(&list->items[i]);
  list->count = 0;
}

void entity_list_free(struct entity_list *list) {
  entity_list_clear(list);
  free(list->items);
  list->items = NULL;
  list->capacity = 0;
}

static int next_entity_id = 1; /* 0 is a reserved "no entity" sentinel */

struct entity *entity_spawn(struct entity_list *list) {
  if (list->count == list->capacity) {
    int newcap = list->capacity ? list->capacity * 2 : 32;
    struct entity *grown = realloc(list->items, (size_t)newcap * sizeof(*grown));
    list->items = grown;
    list->capacity = newcap;
  }
  struct entity *e = &list->items[list->count++];
  memset(e, 0, sizeof(*e));
  e->id = next_entity_id++;
  e->die_frame = -1;
  e->die_after = -1.0;
  e->death_action = DEATH_NONE;
  return e;
}

void entity_randomize_mask(struct entity *e, ascii_rows mask_template) {
  int rows = 0;
  while (mask_template[rows] != NULL) rows++;
  char **owned = malloc((size_t)(rows + 1) * sizeof(*owned));
  for (int i = 0; i < rows; i++) {
    size_t len = strlen(mask_template[i]);
    owned[i] = malloc(len + 1);
    color_randomize_mask(mask_template[i], owned[i]);
  }
  owned[rows] = NULL;
  entity_clear_owned(e);
  e->owned_mask = owned;
}

void entity_set_owned_shape_frames(struct entity *e, char ***rows, int frame_count, double frame_interval_ticks) {
  struct sprite_pair *table = malloc((size_t)frame_count * sizeof(*table));
  for (int i = 0; i < frame_count; i++) {
    table[i].shape = (ascii_rows)rows[i];
    table[i].mask = NULL;
  }
  e->owned_frame_table = table;
  e->owned_shape_rows = rows;
  e->frames = table;
  e->frame_count = frame_count;
  e->frame_cur = 0;
  e->frame_interval = frame_interval_ticks;
}

ascii_rows entity_shape(const struct entity *e) {
  if (e->frames == NULL || e->frame_count <= 0) return NULL;
  int idx = e->frame_cur;
  if (idx < 0 || idx >= e->frame_count) idx = 0;
  return e->frames[idx].shape;
}

ascii_rows entity_mask(const struct entity *e) {
  if (entity_shape(e) == NULL) return NULL;
  if (e->owned_mask != NULL) return (ascii_rows)e->owned_mask;
  int idx = e->frame_cur;
  if (idx < 0 || idx >= e->frame_count) idx = 0;
  return e->frames[idx].mask;
}

int entity_height(const struct entity *e) {
  ascii_rows rows = entity_shape(e);
  if (rows == NULL) return 0;
  int h = 0;
  while (rows[h] != NULL) h++;
  return h;
}

int entity_width(const struct entity *e) {
  ascii_rows rows = entity_shape(e);
  if (rows == NULL) return 0;
  int w = 0;
  for (int i = 0; rows[i] != NULL; i++) {
    int len = (int)strlen(rows[i]);
    if (len > w) w = len;
  }
  return w;
}

static const int periscope_hold_ticks[9] = {1, 4, 4, 9, 9, 9, 4, 4, 1};

static void tick_submarine(struct entity *e, int term_w) {
  double center = term_w / 2.0 - 20.0;
  bool crossing = (e->x < center && e->x + e->vx > center) || (e->x > center && e->x + e->vx < center);
  if (!crossing && e->x != center) {
    e->x += e->vx;
    return;
  }
  if (e->frame_cur < e->frame_count - 1) {
    if (e->frame_timer < periscope_hold_ticks[e->frame_cur]) {
      e->frame_timer += 1.0;
    } else {
      e->frame_timer = 0.0;
      e->frame_cur++;
    }
  } else {
    e->x += e->vx;
  }
}

static void tick_dolphin(struct entity *e) {
  int phase = e->age_ticks % 36;
  double dy;
  if (phase < 14)
    dy = -0.5;
  else if (phase < 16)
    dy = 0.0;
  else if (phase < 30)
    dy = 0.5;
  else
    dy = 0.0;

  e->x += e->vx;
  e->y += dy;
  e->age_ticks++;
}

void entity_tick_all(struct entity_list *list, int term_w, int term_h) {
  for (int i = 0; i < list->count; i++) {
    struct entity *e = &list->items[i];
    if (e->marked_dead) continue;
    if (e->type == ENT_SUBMARINE) {
      tick_submarine(e, term_w);
    } else if (e->type == ENT_LASER || e->type == ENT_FISHHOOK) {
      /* nothing to C here, @scene.c */
    } else {
      if (e->type == ENT_DOLPHIN)
        tick_dolphin(e);
      else {
        e->x += e->vx;
        e->y += e->vy;
      }

      if (e->frame_count > 1 && e->frame_interval > 0.0) {
        e->frame_timer += 1.0;
        if (e->frame_timer >= e->frame_interval) {
          e->frame_timer -= e->frame_interval;
          e->frame_cur = (e->frame_cur + 1) % e->frame_count;
        }
      }
    }
    if (e->die_frame >= 0) {
      e->age_ticks++;
      if (e->age_ticks >= e->die_frame) {
        e->marked_dead = true;
        continue;
      }
    }
    if (e->die_after >= 0.0) {
      e->die_after -= 0.1;
      if (e->die_after <= 0.0) {
        e->marked_dead = true;
        continue;
      }
    }
    if (e->die_offscreen) {
      int w = entity_width(e);
      int h = entity_height(e);
      bool exiting_right = e->vx > 0.0 && e->x > term_w;
      bool exiting_left = e->vx < 0.0 && e->x + w < 0;
      bool exiting_down = e->vy > 0.0 && e->y > term_h;
      bool exiting_up = e->vy < 0.0 && e->y + h < 0;
      if (exiting_right || exiting_left || exiting_down || exiting_up) {
        e->marked_dead = true;
        continue;
      }
    }
  }
}

static bool bbox_overlap(const struct entity *a, const struct entity *b) {
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
        fish->death_action = DEATH_NONE;
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

void entity_reap(struct entity_list *list, entity_death_fn fn, void *ctx) {
  int original_count = list->count;
  int write = 0;

  for (int read = 0; read < original_count; read++) {
    if (list->items[read].marked_dead) {
      struct entity snapshot = list->items[read];
      entity_clear_owned(&list->items[read]);
      if (fn != NULL) fn(&snapshot, ctx);
      continue;
    }
    if (write != read) list->items[write] = list->items[read];
    write++;
  }

  int extra = list->count - original_count;
  if (extra > 0 && write != original_count) memmove(&list->items[write], &list->items[original_count], (size_t)extra * sizeof(*list->items));
  list->count = write + extra;
}

static const struct entity *g_sort_items;

static int cmp_depth(const void *pa, const void *pb) {
  int ia = *(const int *)pa;
  int ib = *(const int *)pb;
  return g_sort_items[ib].z - g_sort_items[ia].z;
}

static bool cell_transparent(const struct entity *e, const char *srow, int len, int col) {
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

bool entity_glyph_overlap(const struct entity *a, const struct entity *b) {
  ascii_rows arows = entity_shape(a);
  ascii_rows brows = entity_shape(b);
  if (arows == NULL || brows == NULL) return false;

  int aw = entity_width(a), ah = entity_height(a);
  int bw = entity_width(b), bh = entity_height(b);
  int ax = round_to_int(a->x), ay = round_to_int(a->y);
  int bx = round_to_int(b->x), by = round_to_int(b->y);

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
    int alen = (int)strlen(asrow);
    int blen = (int)strlen(bsrow);
    for (int wx = x0; wx < x1; wx++) {
      int acol = wx - ax;
      int bcol = wx - bx;
      if (acol < 0 || acol >= alen || bcol < 0 || bcol >= blen) continue;
      if (cell_transparent(a, asrow, alen, acol)) continue;
      if (cell_transparent(b, bsrow, blen, bcol)) continue;
      return true;
    }
  }
  return false;
}

void entity_draw_all(const struct entity_list *list, struct canvas *c) {
  int n = list->count;
  if (n <= 0) return;

  int *order = malloc((size_t)n * sizeof(*order));
  if (order == NULL) return;
  for (int i = 0; i < n; i++) order[i] = i;
  g_sort_items = list->items;
  qsort(order, (size_t)n, sizeof(*order), cmp_depth);
  for (int oi = 0; oi < n; oi++) {
    const struct entity *e = &list->items[order[oi]];
    ascii_rows rows = entity_shape(e);
    if (rows == NULL) continue;
    ascii_rows mrows = entity_mask(e);
    int mask_height = 0;
    if (mrows != NULL) while (mrows[mask_height] != NULL) mask_height++;
    int base_x = round_to_int(e->x);
    int base_y = round_to_int(e->y);
    for (int row = 0; rows[row] != NULL; row++) {
      const char *srow = rows[row];
      int len = (int)strlen(srow);
      const char *mrow = (mrows != NULL && row < mask_height) ? mrows[row] : NULL;
      int mlen = mrow != NULL ? (int)strlen(mrow) : 0;
      for (int col = 0; col < len; col++) {
        if (cell_transparent(e, srow, len, col)) continue;
        struct attr a = e->default_attr;
        if (mrow != NULL && col < mlen && mrow[col] != ' ') a = color_from_mask_letter(mrow[col]);
        canvas_put(c, base_x + col, base_y + row, (unsigned char)srow[col], a);
      }
    }
  }
  free(order);
}
