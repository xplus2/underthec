#include "scene_internal.h"
#include "rng.h"
#include "xalloc.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct aquatic_flag {
  const char *name;
  size_t offset;
};

#define AQ_FLAG(field) {#field, offsetof(struct aquatic_life, field)}

static const struct aquatic_flag aquatic_flags[] = {
  AQ_FLAG(ducks),    AQ_FLAG(dolphins), AQ_FLAG(ship),     AQ_FLAG(swan), AQ_FLAG(kaiju),
  AQ_FLAG(fishhook), AQ_FLAG(submarine),AQ_FLAG(whale),    AQ_FLAG(shark),AQ_FLAG(jellyfish),
  AQ_FLAG(monster),  AQ_FLAG(bigfish),  AQ_FLAG(swordfish),AQ_FLAG(crab),  AQ_FLAG(seahorse),
};
#define AQUATIC_FLAG_COUNT (sizeof(aquatic_flags) / sizeof(aquatic_flags[0]))
_Static_assert(AQUATIC_FLAG_COUNT == SCENE_AQUATIC_FLAG_COUNT, "SCENE_AQUATIC_FLAG_COUNT out of sync");

static bool *aquatic_field(struct aquatic_life *a, size_t offset) {
  return (bool *)((char *)a + offset);
}

void scene_aquatic_fill(struct aquatic_life *a, bool on) {
  for (size_t i = 0; i < AQUATIC_FLAG_COUNT; i++) *aquatic_field(a, aquatic_flags[i].offset) = on;
}

struct aquatic_life scene_aquatic_default(void) {
  struct aquatic_life a;
  a.fish_count = -1;
  scene_aquatic_fill(&a, true);
  return a;
}

struct aquatic_life scene_aquatic_classic11(void) {
  return (struct aquatic_life){
      .fish_count = -1,
      .ship = true,
      .whale = true,
      .monster = true,
      .bigfish = true,
      .shark = true,
  };
}

bool scene_aquatic_set_flag(struct aquatic_life *a, const char *name) {
  for (size_t i = 0; i < AQUATIC_FLAG_COUNT; i++) {
    if (strcmp(name, aquatic_flags[i].name) == 0) {
      *aquatic_field(a, aquatic_flags[i].offset) = true;
      return true;
    }
  }
  return false;
}

const char *scene_aquatic_flag_name(size_t i) { return aquatic_flags[i].name; }

bool *scene_aquatic_flag(struct aquatic_life *a, size_t i) { return aquatic_field(a, aquatic_flags[i].offset); }

struct scene_ctx {
  struct scene *sc;
  int w;
  int h;
};

static void on_death(const struct entity *dead, void *ctx) {
  struct scene_ctx *sctx = ctx;

  if (dead->spawn_splat) spawn_splat(sctx->sc, dead->splat_x, dead->splat_y, dead->splat_z);
  if (dead->type == ENT_KAIJU && dead->id == sctx->sc->castle_hidden_by) {
    add_castle_building(sctx->sc, sctx->w, sctx->h);
    sctx->sc->castle_hidden_by = 0;
  }
  switch (dead->death_action) {
    case DEATH_ADD_FISH:            spawn_fish(sctx->sc, sctx->w, sctx->h);          break;
    case DEATH_ADD_SEAWEED:         add_seaweed(sctx->sc, sctx->w, sctx->h);         break;
    /* respawn after a while: this is how actual sequels to kaiju movies work */
    case DEATH_ADD_KAIJU:           spawn_kaiju(sctx->sc, sctx->w, sctx->h);         break;
    case DEATH_ADD_KAIJU_COOLDOWN:  schedule_kaiju_return(sctx->sc);                 break;
    case DEATH_RANDOM_OBJECT:
    case DEATH_SHARK:               schedule_random_object_return(sctx->sc);         break;
    case DEATH_ADD_RANDOM_OBJECT:   spawn_random_object(sctx->sc, sctx->w, sctx->h); break;
    case DEATH_ADD_MESSAGE:         add_message(sctx->sc, sctx->w, sctx->h);         break;
    case DEATH_NONE:
    default:                                                                         break;
  }
}

void scene_init(struct scene *sc, bool classic_mode, struct aquatic_life aquatic) {
  entity_list_init(&sc->entities);
  sc->classic_mode = classic_mode;
  sc->aquatic = aquatic;
  sc->castle = true;
  sc->message_rows = NULL;
  sc->message_frame.shape = NULL;
  sc->message_frame.mask = NULL;
  sc->message_attr = color_from_name("blue");
  sc->message_position = MSG_POS_MIDDLE;
  sc->uturn_chance = 200;
  sc->feed_alerted = true;
}

void scene_free(struct scene *sc) {
  entity_list_free(&sc->entities);
  scene_set_message(sc, NULL, 0);
  entity_draw_shutdown();
}

void scene_reset(struct scene *sc, int term_w, int term_h) {
  entity_list_clear(&sc->entities);
  sc->castle_hidden_by = 0;
  sc->feed_alerted = true;
  if (sc->message_rows != NULL) add_message(sc, term_w, term_h);
  add_environment(sc, term_w, term_h);
  if (sc->castle) add_castle(sc, term_w, term_h);
  add_all_seaweed(sc, term_w, term_h);
  add_all_fish(sc, term_w, term_h);
  if (sc->aquatic.kaiju) schedule_kaiju_return(sc);
  spawn_random_object(sc, term_w, term_h);
  spawn_jellyfish(sc, term_w, term_h);
}

void scene_tick(struct scene *sc, int term_w, int term_h) {
  entity_tick_all(&sc->entities, term_w, term_h);
  environment_tick(sc);
  for (int i = 0; i < sc->entities.count; i++) {
    struct entity *fish = &sc->entities.items[i];
    if (fish->marked_dead || fish->type != ENT_FISH) continue;
    fish_turn_tick(fish, term_w, sc->uturn_chance);
    if (rng_int(100) > 97) {
      int fw = entity_width(fish);
      int fh = entity_height(fish);
      spawn_bubble(sc, fish->x, fish->y, fish->z, fw, fh, fish->vx);
    }
  }

  if (rng_int(1200) == 0) spawn_jellyfish(sc, term_w, term_h);
  kaiju_tick(sc, term_w);
  castle_door_tick(sc);
  fishhook_tick(sc, term_h);
  feed_tick(sc, term_w, term_h);
  entity_collide_all(&sc->entities);
  bool shark_died = false;
  for (int i = 0; i < sc->entities.count; i++) {
    if (sc->entities.items[i].marked_dead && sc->entities.items[i].type == ENT_SHARK) {
      shark_died = true;
      break;
    }
  }
  if (shark_died) for (int i = 0; i < sc->entities.count; i++)
    if (sc->entities.items[i].type == ENT_TEETH) sc->entities.items[i].marked_dead = true;
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

void scene_set_message_color(struct scene *sc, struct attr attr) {
  sc->message_attr = attr;
}

void scene_set_uturn_chance(struct scene *sc, int one_in) {
  sc->uturn_chance = one_in;
}

void scene_set_castle(struct scene *sc, bool on) { sc->castle = on; }

void scene_feed(struct scene *sc, int w, int h) {
  feed_trigger(sc, w, h);
}

int scene_fish_display_count(const struct scene *sc) {
  int n = 0;
  for (int i = 0; i < sc->entities.count; i++) {
    const struct entity *e = &sc->entities.items[i];
    if (!e->marked_dead && e->type == ENT_FISH) n++;
  }
  return n;
}

void scene_set_fish_count(struct scene *sc, int w, int h, int count) {
  if (count < 0) count = 0;
  if (count > 100000) count = 100000;
  sc->aquatic.fish_count = count;
  int live = scene_fish_display_count(sc);
  if (live < count) {
    for (int i = live; i < count; i++) spawn_fish(sc, w, h);
    return;
  }
  int to_kill = live - count;
  for (int i = 0; i < sc->entities.count && to_kill > 0; i++) {
    struct entity *e = &sc->entities.items[i];
    if (e->marked_dead || e->type != ENT_FISH) continue;
    e->marked_dead = true;
    e->death_action = DEATH_NONE;
    to_kill--;
  }
  struct scene_ctx ctx = {sc, w, h};
  entity_reap(&sc->entities, on_death, &ctx);
}

void scene_on_species_toggled(struct scene *sc, int w, int h) {
  if (sc->aquatic.kaiju) {
    bool has_timer = false;
    bool has_live = false;
    for (int i = 0; i < sc->entities.count; i++) {
      const struct entity *e = &sc->entities.items[i];
      if (e->marked_dead) continue;
      if (e->type == ENT_KAIJU_TIMER) has_timer = true;
      if (e->type == ENT_KAIJU) has_live = true;
    }
    if (!has_timer && !has_live) schedule_kaiju_return(sc);
  }

  bool pool_enabled = sc->aquatic.ship || sc->aquatic.whale || sc->aquatic.monster || sc->aquatic.bigfish ||
                      sc->aquatic.shark || sc->aquatic.submarine || sc->aquatic.swordfish || sc->aquatic.ducks ||
                      sc->aquatic.dolphins || sc->aquatic.swan || sc->aquatic.fishhook || sc->aquatic.crab ||
                      sc->aquatic.seahorse;
  if (pool_enabled) {
    bool has_timer = false;
    bool has_live = false;
    for (int i = 0; i < sc->entities.count; i++) {
      const struct entity *e = &sc->entities.items[i];
      if (e->marked_dead) continue;
      if (e->type == ENT_RANDOM_OBJECT_TIMER) has_timer = true;
      switch (e->type) {
        case ENT_SHIP: case ENT_WHALE: case ENT_MONSTER: case ENT_BIGFISH: case ENT_SHARK:
        case ENT_SUBMARINE: case ENT_SWORDFISH: case ENT_DUCK: case ENT_DOLPHIN: case ENT_SWAN:
        case ENT_FISHHOOK: case ENT_CRAB: case ENT_SEAHORSE:
          has_live = true;
          break;
        default:
          break;
      }
    }
    if (!has_timer && !has_live) spawn_random_object(sc, w, h);
  }

  if (sc->aquatic.jellyfish) {
    bool has_live = false;
    for (int i = 0; i < sc->entities.count; i++) {
      if (!sc->entities.items[i].marked_dead && sc->entities.items[i].type == ENT_JELLYFISH) {
        has_live = true;
        break;
      }
    }
    if (!has_live) spawn_jellyfish(sc, w, h);
  }
}

void scene_set_message_position(struct scene *sc, enum message_position pos) {
  sc->message_position = pos;
}

void scene_draw(const struct scene *sc, struct canvas *c, double alpha) {
  entity_draw_all(&sc->entities, c, alpha);
}
