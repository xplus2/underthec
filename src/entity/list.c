#include "priv.h"
#include "xalloc.h"

#include <stdlib.h>
#include <string.h>

void entity_free_owned_rows(char **rows) {
  if (rows == NULL) return;
  for (int i = 0; rows[i] != NULL; i++) free(rows[i]);
  free(rows);
}

void entity_clear_owned_mask_frames(struct entity *e) {
  if (e->owned_mask_frames == NULL) return;
  for (int i = 0; i < e->frame_count; i++) entity_free_owned_rows(e->owned_mask_frames[i]);
  free(e->owned_mask_frames);
  e->owned_mask_frames = NULL;
  if (e->owned_frame_table != NULL) for (int i = 0; i < e->frame_count; i++) e->owned_frame_table[i].mask = NULL;
}

void entity_clear_owned(struct entity *e) {
  if (e->owned_mask != NULL) {
    entity_free_owned_rows(e->owned_mask);
    e->owned_mask = NULL;
  }
  if (e->owned_frame_table != NULL) {
    for (int i = 0; i < e->frame_count; i++) entity_free_owned_rows(e->owned_shape_rows[i]);
    free(e->owned_shape_rows);
    e->owned_shape_rows = NULL;
    entity_clear_owned_mask_frames(e);
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

static int next_entity_id = 1; /* 0 reserved "no entity" */

struct entity *entity_spawn(struct entity_list *list) {
  if (list->count == list->capacity) {
    int newcap = list->capacity ? list->capacity * 2 : 32;
    struct entity *grown = xrealloc(list->items, (size_t)newcap * sizeof(*grown));
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

struct entity *entity_find_first(struct entity_list *list, enum entity_type type) {
  for (int i = 0; i < list->count; i++) {
    struct entity *e = &list->items[i];
    if (!e->marked_dead && e->type == type) return e;
  }
  return NULL;
}

struct entity *entity_find_by_id(struct entity_list *list, int id) {
  for (int i = 0; i < list->count; i++) {
    struct entity *e = &list->items[i];
    if (!e->marked_dead && e->id == id) return e;
  }
  return NULL;
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
