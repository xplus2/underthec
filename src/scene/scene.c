#include "scene_internal.h"
#include "rng.h"
#include "xalloc.h"

#include <stdlib.h>
#include <string.h>

struct scene_ctx {
  struct scene *sc;
  int w, h;
};

static void on_death(const struct entity *dead, void *ctx) {
  struct scene_ctx *sctx = ctx;

  if (dead->spawn_splat) spawn_splat(sctx->sc, dead->splat_x, dead->splat_y, dead->splat_z);
  if (dead->type == ENT_KAIJU && dead->id == sctx->sc->castle_hidden_by) {
    add_castle_building(sctx->sc, sctx->w, sctx->h);
    sctx->sc->castle_hidden_by = 0;
  }
  switch (dead->death_action) {
  case DEATH_ADD_FISH:
    spawn_fish(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_ADD_SEAWEED:
    add_seaweed(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_ADD_KAIJU:
    spawn_kaiju(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_ADD_KAIJU_COOLDOWN:
    schedule_kaiju_return(sctx->sc);
    break;
  case DEATH_RANDOM_OBJECT:
  case DEATH_SHARK:
    spawn_random_object(sctx->sc, sctx->w, sctx->h);
    break;
  case DEATH_NONE:
  default:
    break;
  }
}

void scene_init(struct scene *sc, bool classic_mode) {
  entity_list_init(&sc->entities);
  sc->classic_mode = classic_mode;
  sc->message_rows = NULL;
  sc->message_frame.shape = NULL;
  sc->message_frame.mask = NULL;
}

void scene_free(struct scene *sc) {
  entity_list_free(&sc->entities);
  scene_set_message(sc, NULL, 0);
  entity_draw_shutdown();
}

void scene_reset(struct scene *sc, int term_w, int term_h) {
  entity_list_clear(&sc->entities);
  sc->castle_hidden_by = 0;

  if (sc->message_rows != NULL) add_message(sc, term_w, term_h);
  add_environment(sc, term_w, term_h);
  add_castle(sc, term_w, term_h);
  add_all_seaweed(sc, term_w, term_h);
  add_all_fish(sc, term_w, term_h);
  spawn_kaiju(sc, term_w, term_h);
  spawn_random_object(sc, term_w, term_h);
  spawn_jellyfish(sc, term_w, term_h);
}

void scene_tick(struct scene *sc, int term_w, int term_h) {
  entity_tick_all(&sc->entities, term_w, term_h);
  environment_tick(sc);

  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    if (rng_int(100) > 97) {
      int fw = entity_width(fish);
      int fh = entity_height(fish);
      spawn_bubble(sc, fish->x, fish->y, fish->z, fw, fh, fish->vx);
    }
  }

  if (rng_int(200) == 0) spawn_jellyfish(sc, term_w, term_h);

  kaiju_tick(sc, term_w);
  fishhook_tick(sc, term_h);

  entity_collide_all(&sc->entities);
  bool shark_died = false;
  for (int i = 0; i < sc->entities.count; i++) {
    if (sc->entities.items[i].marked_dead &&
        sc->entities.items[i].type == ENT_SHARK) {
      shark_died = true;
      break;
    }
  }
  if (shark_died) {
    for (int i = 0; i < sc->entities.count; i++) {
      if (sc->entities.items[i].type == ENT_TEETH) sc->entities.items[i].marked_dead = true;
    }
  }

  struct scene_ctx ctx = {sc, term_w, term_h};
  entity_reap(&sc->entities, on_death, &ctx);
}

void scene_set_message(struct scene *sc, const char *const *rows, int row_count) {
  if (sc->message_rows != NULL) {
    for (int i = 0; sc->message_rows[i] != NULL; i++) free(sc->message_rows[i]);
    free(sc->message_rows);
    sc->message_rows = NULL;
    sc->message_frame.shape = NULL;
  }

  if (rows == NULL || row_count <= 0) return;
  char **copy = xmalloc((size_t)(row_count + 1) * sizeof(*copy));
  for (int i = 0; i < row_count; i++) {
    size_t len = strlen(rows[i]);
    copy[i] = xmalloc(len + 1);
    memcpy(copy[i], rows[i], len + 1);
  }
  copy[row_count] = NULL;
  sc->message_rows = copy;
  sc->message_frame.shape = (ascii_rows)sc->message_rows;
  sc->message_frame.mask = NULL;
}

void scene_draw(const struct scene *sc, struct canvas *c) {
  entity_draw_all(&sc->entities, c);
}
