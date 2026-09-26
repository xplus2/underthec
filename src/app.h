#ifndef UNDERTHEC_APP_H
#define UNDERTHEC_APP_H

#include <stdbool.h>

#include "canvas.h"
#include "ui/help.h"
#include "scene.h"
#include "ui/settings.h"

/* dialogs point into struct, no copies after init */
struct app {
  struct scene scene;
  struct canvas canvas;
  struct settings_ui settings;
  struct help_ui help;
  int w;
  int h;
  bool paused;
  double tick_accum;
  double pace;
  int fps;
  double last;
};

/* now=secs */
void app_init(struct app *a, bool classic, struct aquatic_life aquatic, double pace, int fps, double now);
void app_free(struct app *a);

void app_resize(struct app *a, int w, int h);

/* r,p,f,s,h,esc + dialog keys */
void app_key(struct app *a, int key);
/* column: click/tap pos or FEED_COL_AUTO */
void app_feed(struct app *a, int col);
/* dialog hits, otherwise: feed */
void app_click(struct app *a, int x, int y);

/* tick, canvas redraw */
void app_frame(struct app *a, double now);

#endif
