#include "../app.h"
#include "../config.h"
#include "../xalloc.h"

#include <stdint.h>

/* JS calls */
const char *web_opt(const char *name, const char *value, const char *shown);
const char *web_init(double now_ms);
int web_fps(void);
void web_resize(int cols, int rows);
void web_key(int key);
void web_click(int col, int row);
const uint32_t *web_frame(double now_ms);
int web_width(void);
int web_height(void);

static struct app app;
static uint32_t *packed;
static size_t packed_cap;
static char err[256];
static bool cfg_ready;
static struct config cfg;

static uint32_t utf8_codepoint(const char *s) {
  const unsigned char *u = (const unsigned char *)s;
  if (u[0] < 0x80) return u[0];
  if ((u[0] & 0xe0) == 0xc0) return ((uint32_t)(u[0] & 0x1f) << 6) | (u[1] & 0x3f);
  if ((u[0] & 0xf0) == 0xe0) return ((uint32_t)(u[0] & 0x0f) << 12) | ((uint32_t)(u[1] & 0x3f) << 6) | (u[2] & 0x3f);
  return ((uint32_t)(u[0] & 0x07) << 18) | ((uint32_t)(u[1] & 0x3f) << 12) | ((uint32_t)(u[2] & 0x3f) << 6) | (u[3] & 0x3f);
}

static void cfg_defaults(void) {
  if (cfg_ready) return;
  config_init(&cfg);
  cfg_ready = true;
}

const char *web_opt(const char *name, const char *value, const char *shown) {
  cfg_defaults();
  return config_set(&cfg, name, value, shown, err, sizeof err) ? NULL : err;
}

const char *web_init(double now_ms) {
  cfg_defaults();
  if (!config_check(&cfg, err, sizeof err)) return err;
  config_start(&cfg, &app, now_ms / 1000.0);
  config_free(&cfg);
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

void web_click(int col, int row) { app_click(&app, col, row); }

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
