#ifndef UNDERTHEC_CONFIG_H
#define UNDERTHEC_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "app.h"
#include "scene.h"

/* options by long CLI name */
struct config {
  int classic_ver; /* 0=off, 1=1.0, 2=1.1 */
  bool classic_given;
  bool aquatic_given;
  struct aquatic_life aquatic;
  char *message;
  char *message_color;
  char *castle_name;
  bool no_castle;
  enum message_position message_position;
  double pace;
  int uturn_chance;
  int fps;
};

void config_init(struct config *cfg);
void config_free(struct config *cfg);

/* name in errors. false: err set */
bool config_set(struct config *cfg, const char *name, const char *value, const char *shown, char *err, size_t err_len);
/* cross options */
bool config_check(const struct config *cfg, char *err, size_t err_len);
/* rng, app init */
void config_start(const struct config *cfg, struct app *app, double now);

#endif
