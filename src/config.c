#include "config.h"
#include "color.h"
#include "opts.h"
#include "rng.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void config_init(struct config *cfg) {
  *cfg = (struct config){
      .aquatic = scene_aquatic_default(),
      .message_position = MSG_POS_MIDDLE,
      .pace = 1.0,
      .uturn_chance = 200,
      .fps = 10,
  };
}

void config_free(struct config *cfg) {
  free(cfg->message);
  free(cfg->message_color);
  free(cfg->castle_name);
  cfg->message = NULL;
  cfg->message_color = NULL;
  cfg->castle_name = NULL;
}

static bool fail(char *err, size_t err_len, const char *msg, const char *shown) {
  opts_set_errbuf(err, err_len, (const char *[]){msg, " for ", shown}, 3);
  return false;
}

bool config_set(struct config *cfg, const char *name, const char *value, const char *shown, char *err, size_t err_len) {
  char eb[128];
  if (strcmp(name, "classic") == 0) {
    if (!opts_parse_classic(value, &cfg->classic_ver, eb, sizeof eb)) return fail(err, err_len, eb, shown);
    cfg->classic_given = true;
  } else if (strcmp(name, "aquatic-life") == 0) {
    bool fish_set = false;
    if (!opts_parse_aquatic_life(value, &cfg->aquatic, true, &fish_set, eb, sizeof eb)) return fail(err, err_len, eb, shown);
    cfg->aquatic_given = true;
  } else if (strcmp(name, "message") == 0) {
    free(cfg->message);
    cfg->message = opts_strdup(value);
  } else if (strcmp(name, "message-color") == 0) {
    if (!color_name_valid(value)) {
      opts_set_errbuf(eb, sizeof eb, (const char *[]){"invalid color '", value, "'"}, 3);
      return fail(err, err_len, eb, shown);
    }
    free(cfg->message_color);
    cfg->message_color = opts_strdup(value);
  } else if (strcmp(name, "message-position") == 0) {
    if (!opts_parse_message_position(value, &cfg->message_position, eb, sizeof eb)) return fail(err, err_len, eb, shown);
  } else if (strcmp(name, "castle-name") == 0) {
    char name_buf[CASTLE_NAME_LEN + 1];
    if (!opts_parse_castle_name(value, name_buf, sizeof name_buf, eb, sizeof eb)) return fail(err, err_len, eb, shown);
    free(cfg->castle_name);
    cfg->castle_name = opts_strdup(name_buf);
  } else if (strcmp(name, "pace") == 0) {
    if (!opts_parse_pace(value, &cfg->pace, eb, sizeof eb)) return fail(err, err_len, eb, shown);
  } else if (strcmp(name, "uturn-chance") == 0) {
    if (!opts_parse_uturn_chance(value, &cfg->uturn_chance, eb, sizeof eb)) return fail(err, err_len, eb, shown);
  } else if (strcmp(name, "fps") == 0) {
    if (!opts_parse_fps(value, &cfg->fps, eb, sizeof eb)) return fail(err, err_len, eb, shown);
  } else {
    return fail(err, err_len, "unknown option", shown);
  }
  return true;
}

bool config_check(const struct config *cfg, char *err, size_t err_len) {
  if (cfg->classic_given && cfg->aquatic_given) {
    opts_set_errbuf(err, err_len, (const char *[]){"classic and aquatic-life are mutually exclusive"}, 1);
    return false;
  }
  return true;
}

void config_start(const struct config *cfg, struct app *app, double now) {
  struct aquatic_life aquatic = cfg->classic_ver == 2 ? scene_aquatic_classic11() : cfg->aquatic;
  rng_seed((uint64_t)time(NULL) ^ ((uint64_t)clock() << 32));
  app_init(app, cfg->classic_ver == 1, aquatic, cfg->pace, cfg->fps, now);
  if (cfg->message_color != NULL) scene_set_message_color(&app->scene, color_from_name(cfg->message_color));
  scene_set_message_position(&app->scene, cfg->message_position);
  scene_set_uturn_chance(&app->scene, cfg->uturn_chance);
  if (cfg->castle_name != NULL) scene_set_castle_name(&app->scene, cfg->castle_name);
  if (cfg->message != NULL) {
    char *buf = opts_strdup(cfg->message);
    char **rows = NULL;
    int count = opts_split_lines(buf, &rows);
    if (count > 0) scene_set_message(&app->scene, (const char *const *)rows, count);
    free(rows);
    free(buf);
  }
}
