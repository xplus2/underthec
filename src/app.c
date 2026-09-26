#include "app.h"

void app_init(struct app *a, bool classic, struct aquatic_life aquatic, double pace, int fps, double now) {
  scene_init(&a->scene, classic, aquatic);
  canvas_init(&a->canvas);
  a->w = -1;
  a->h = -1;
  a->paused = false;
  a->tick_accum = 0.0;
  a->pace = pace;
  a->fps = fps;
  a->last = now;
  settings_ui_init(&a->settings, &a->fps, &a->pace, &a->scene);
  help_ui_init(&a->help, &a->fps, &a->pace);
}

void app_free(struct app *a) {
  canvas_free(&a->canvas);
  scene_free(&a->scene);
}

void app_resize(struct app *a, int w, int h) {
  if (w == a->w && h == a->h) return;
  canvas_resize(&a->canvas, w, h);
  scene_reset(&a->scene, w, h);
  a->w = w;
  a->h = h;
}

void app_key(struct app *a, int key) {
  if (key == 'r') scene_reset(&a->scene, a->w, a->h);
  if (key == 'p') a->paused = !a->paused;
  if (key == 'f') app_feed(a, FEED_COL_AUTO);
  if (key == 's') {
    if (help_ui_is_open(&a->help)) help_ui_close(&a->help);
    settings_ui_toggle(&a->settings);
  }
  if (key == 'h') {
    if (settings_ui_is_open(&a->settings)) settings_ui_close(&a->settings);
    help_ui_toggle(&a->help);
  }
  if (settings_ui_is_open(&a->settings)) {
    if (key == '\x1b') settings_ui_close(&a->settings);
    else settings_ui_handle_key(&a->settings, key, a->w, a->h);
  }
  if (help_ui_is_open(&a->help) && key == '\x1b') help_ui_close(&a->help);
}

void app_feed(struct app *a, int col) { scene_feed(&a->scene, a->w, a->h, col); }

void app_click(struct app *a, int x, int y) {
  if (settings_ui_click(&a->settings, x, y, a->w, a->h)) return;
  app_feed(a, x);
}

void app_frame(struct app *a, double now) {
  double dt = now - a->last;
  a->last = now;
  if (dt < 0.0) dt = 0.0;
  if (dt > 0.5) dt = 0.5;
  if (!a->paused) {
    a->tick_accum += dt * 10.0 * a->pace;
    while (a->tick_accum >= 1.0) {
      scene_tick(&a->scene, a->w, a->h);
      a->tick_accum -= 1.0;
    }
  }
  canvas_clear(&a->canvas);
  scene_draw(&a->scene, &a->canvas, a->tick_accum);
  settings_ui_draw(&a->settings, &a->canvas);
  help_ui_draw(&a->help, &a->canvas);
}
