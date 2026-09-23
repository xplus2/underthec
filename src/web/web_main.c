#include "../app.h"
#include "../color.h"
#include "../opts.h"
#include "../rng.h"
#include "../xalloc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* JS calls */
const char *web_opt(const char *name, const char *value, const char *shown);
const char *web_init(double now_ms);
int web_fps(void);
void web_resize(int cols, int rows);
void web_key(int key);
void web_feed(void);
const uint32_t *web_frame(double now_ms);
int web_width(void);
int web_height(void);

static struct app app;
static uint32_t *packed;
static size_t packed_cap;
static char err[256];
static bool opt_ready;
static int classic_ver;
static bool classic_given;
static bool aquatic_given;
static struct aquatic_life aquatic;
static char *message;
static char *message_color;
static enum message_position message_position = MSG_POS_MIDDLE;
static double pace = 1.0;
static int uturn_chance = 200;
static int fps = 10;

static uint32_t utf8_codepoint(const char *s) {
  const unsigned char *u = (const unsigned char *)s;
  if (u[0] < 0x80) return u[0];
  if ((u[0] & 0xe0) == 0xc0) return ((uint32_t)(u[0] & 0x1f) << 6) | (u[1] & 0x3f);
  if ((u[0] & 0xf0) == 0xe0) return ((uint32_t)(u[0] & 0x0f) << 12) | ((uint32_t)(u[1] & 0x3f) << 6) | (u[2] & 0x3f);
  return ((uint32_t)(u[0] & 0x07) << 18) | ((uint32_t)(u[1] & 0x3f) << 12) | ((uint32_t)(u[2] & 0x3f) << 6) | (u[3] & 0x3f);
}

static void opt_defaults(void) {
  if (opt_ready) return;
  aquatic = scene_aquatic_default();
  opt_ready = true;
}

static const char *fail(const char *msg, const char *shown) {
  opts_set_errbuf(err, sizeof err, (const char *[]){msg, " for ", shown}, 3);
  return err;
}

const char *web_opt(const char *name, const char *value, const char *shown) {
  char eb[128];
  opt_defaults();
  if (strcmp(name, "classic") == 0) {
    if (!opts_parse_classic(value, &classic_ver, eb, sizeof eb)) return fail(eb, shown);
    classic_given = true;
  } else if (strcmp(name, "aquatic-life") == 0) {
    bool fish_set = false;
    if (!opts_parse_aquatic_life(value, &aquatic, true, &fish_set, eb, sizeof eb)) return fail(eb, shown);
    aquatic_given = true;
  } else if (strcmp(name, "message") == 0) {
    free(message);
    message = opts_strdup(value);
  } else if (strcmp(name, "message-color") == 0) {
    if (!color_name_valid(value)) {
      opts_set_errbuf(eb, sizeof eb, (const char *[]){"invalid color '", value, "'"}, 3);
      return fail(eb, shown);
    }
    free(message_color);
    message_color = opts_strdup(value);
  } else if (strcmp(name, "message-position") == 0) {
    if (!opts_parse_message_position(value, &message_position, eb, sizeof eb)) return fail(eb, shown);
  } else if (strcmp(name, "pace") == 0) {
    if (!opts_parse_pace(value, &pace, eb, sizeof eb)) return fail(eb, shown);
  } else if (strcmp(name, "uturn-chance") == 0) {
    if (!opts_parse_uturn_chance(value, &uturn_chance, eb, sizeof eb)) return fail(eb, shown);
  } else if (strcmp(name, "fps") == 0) {
    if (!opts_parse_fps(value, &fps, eb, sizeof eb)) return fail(eb, shown);
  } else {
    return fail("unknown option", shown);
  }
  return NULL;
}

const char *web_init(double now_ms) {
  opt_defaults();
  if (classic_given && aquatic_given) {
    opts_set_errbuf(err, sizeof err, (const char *[]){"classic and aquatic-life are mutually exclusive"}, 1);
    return err;
  }
  if (classic_ver == 2) aquatic = scene_aquatic_classic11();
  rng_seed((uint64_t)time(NULL) ^ ((uint64_t)clock() << 32));
  app_init(&app, classic_ver == 1, aquatic, pace, fps, now_ms / 1000.0);
  if (message_color != NULL) scene_set_message_color(&app.scene, color_from_name(message_color));
  scene_set_message_position(&app.scene, message_position);
  scene_set_uturn_chance(&app.scene, uturn_chance);
  if (message != NULL) {
    char **rows = NULL;
    int count = opts_split_lines(message, &rows);
    if (count > 0) scene_set_message(&app.scene, (const char *const *)rows, count);
    free(rows);
  }
  free(message);
  message = NULL;
  free(message_color);
  message_color = NULL;
  return NULL;
}

/* settings dialog changes it */
int web_fps(void) { return app.fps; }

void web_resize(int cols, int rows) {
  app_resize(&app, cols, rows);
  size_t n = (size_t)app.canvas.width * (size_t)app.canvas.height;
  if (n > packed_cap) {
    packed = xrealloc(packed, n * sizeof(*packed));
    packed_cap = n;
  }
}

void web_key(int key) { app_key(&app, key); }

void web_feed(void) { app_feed(&app); }

const uint32_t *web_frame(double now_ms) {
  app_frame(&app, now_ms / 1000.0);
  size_t n = (size_t)app.canvas.width * (size_t)app.canvas.height;
  for (size_t i = 0; i < n; i++) {
    const struct cell *c = &app.canvas.cells[i];
    uint32_t cp = c->cont ? 0 : utf8_codepoint(c->glyph);
    packed[i] = cp << 10 | (c->bg_bold ? 1u : 0u) << 9 | (uint32_t)c->bg << 5 | (uint32_t)c->col << 1 | (c->bold ? 1u : 0u);
  }
  return packed;
}

int web_width(void) { return app.canvas.width; }

int web_height(void) { return app.canvas.height; }
