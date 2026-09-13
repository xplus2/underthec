#include "canvas.h"
#include "color.h"
#include "rng.h"
#include "scene.h"
#include "term/term.h"
#include "version.h"
#include "xalloc.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t g_should_quit = 0;

static void on_signal(int sig) {
  (void)sig;
  g_should_quit = 1;
}

static void append_bounded(char *dst, size_t dst_cap, size_t *pos, const char *src) {
  size_t src_len = strlen(src);
  size_t avail = dst_cap > *pos ? dst_cap - *pos : 0;
  if (src_len > avail) src_len = avail;
  memcpy(dst + *pos, src, src_len);
  *pos += src_len;
}

static void write_parts(FILE *stream, const char *const *parts, size_t count) {
  char buf[512] = {0};
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) append_bounded(buf, sizeof(buf), &pos, parts[i]);
  fwrite(buf, 1, pos, stream);
}

static void print_help(const char *prog) {
  write_parts(stdout, (const char *[]){"usage: ", prog, " [options]\n\n"}, 3);
  fputs(
    "  -a, --aquatic-life <definition>\n"
    "                        comma-separated, default: all on, fish=auto\n"
    "                        fish=<N|auto>,ducks,dolphins,ship,swan,kaiju,crab,shark,\n"
    "                        submarine,whale,jellyfish,monster,bigfish,swordfish\n"
    "  -c, --classic [1.0|1.1]\n"
    "                        classic mode, no arg = 1.0\n"
    "  -m, --message <text>  bg text/ascii art ('-' for stdin)\n"
    "  -M, --message-color <color>\n"
    "                        -m text color (default: blue)\n"
    "                        red,green,blue,yellow,magenta,cyan,white,black\n"
    "                        capitalized first letter=bold\n"
    "  -p, --pace <pace>     speed multiplier, 0.01-10 (default: 1)\n"
    "  -s, --screensaver     exit on any keypress\n"
    "  -t, --transparent     transparent background (default: opaque black)\n"
    "  -h, --help            show this help\n"
    "  -v, --version         show version\n\n"
    "keys while running: q quit, r redraw, p pause, t toggle transparency\n\n"
    "environment variables:\n"
    "  UNDERTHEC_FISH=auto|number       like -a's fish=\n"
    "  UNDERTHEC_AQUATIC_LIFE=<def>     like -a, except for fish=\n"
    "  UNDERTHEC_CLASSIC=1.0|1.1        like -c\n"
    "  UNDERTHEC_MESSAGE=<text>         like -m\n"
    "  UNDERTHEC_MESSAGE_COLOR=<color>  like -M\n"
    "  UNDERTHEC_PACE=<pace>            like -p\n",
    "  UNDERTHEC_SCREENSAVER=0|1        like -s\n"
    "  UNDERTHEC_TRANSPARENT=0|1        like -t\n"
    stdout);
}

static void set_errbuf(char *errbuf, size_t errbuf_len, const char *const *parts, size_t count) {
  if (errbuf_len == 0) return;
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) append_bounded(errbuf, errbuf_len - 1, &pos, parts[i]);
  errbuf[pos] = '\0';
}

static int err_requires_arg(const char *prog, const char *opt) {
  write_parts(stderr, (const char *[]){prog, ": ", opt, " requires an argument\n"}, 4);
  return 2;
}

static char *owned_copy(const char *s) {
  size_t len = strlen(s);
  char *p = xmalloc(len + 1);
  memcpy(p, s, len + 1);
  return p;
}

struct aquatic_life_flag {
  const char *name;
  size_t offset;
};

#define AQ_FLAG(field) {#field, offsetof(struct aquatic_life, field)}

static const struct aquatic_life_flag aquatic_life_flags[] = {
  AQ_FLAG(ducks),    AQ_FLAG(dolphins), AQ_FLAG(ship),     AQ_FLAG(swan), AQ_FLAG(kaiju),
  AQ_FLAG(fishhook), AQ_FLAG(submarine),AQ_FLAG(whale),    AQ_FLAG(shark),AQ_FLAG(jellyfish),
  AQ_FLAG(monster),  AQ_FLAG(bigfish),  AQ_FLAG(swordfish),AQ_FLAG(crab),
};
#define AQUATIC_LIFE_FLAG_COUNT (sizeof(aquatic_life_flags) / sizeof(aquatic_life_flags[0]))

static bool *aquatic_life_field(struct aquatic_life *a, size_t offset) {
  return (bool *)((char *)a + offset);
}

static struct aquatic_life aquatic_life_default(void) {
  struct aquatic_life a;
  a.fish_count = -1;
  for (size_t i = 0; i < AQUATIC_LIFE_FLAG_COUNT; i++) *aquatic_life_field(&a, aquatic_life_flags[i].offset) = true;
  return a;
}

static bool aquatic_life_set_flag(struct aquatic_life *out, const char *name) {
  for (size_t i = 0; i < AQUATIC_LIFE_FLAG_COUNT; i++) {
    if (strcmp(name, aquatic_life_flags[i].name) == 0) {
      *aquatic_life_field(out, aquatic_life_flags[i].offset) = true;
      return true;
    }
  }
  return false;
}

static bool parse_fish_count(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "auto") == 0) {
    *out = -1;
    return true;
  }
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 0 || n > 100000) {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fish count '", val, "'"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

static bool aquatic_life_parse(const char *definition, struct aquatic_life *out, bool allow_fish, bool *fish_set, char *errbuf, size_t errbuf_len) {
  for (size_t i = 0; i < AQUATIC_LIFE_FLAG_COUNT; i++) *aquatic_life_field(out, aquatic_life_flags[i].offset) = false;
  char *buf = owned_copy(definition);
  size_t len = strlen(buf);
  bool ok = true;
  const char *token = buf;
  for (size_t i = 0; i <= len && ok; i++) {
    if (buf[i] != ',' && buf[i] != '\0') continue;
    buf[i] = '\0';
    if (token[0] == '\0') {
      set_errbuf(errbuf, errbuf_len, (const char *[]){"empty entry in aquatic-life definition"}, 1);
      ok = false;
    } else if (strncmp(token, "fish=", 5) == 0) {
      if (!allow_fish) {
        set_errbuf(errbuf, errbuf_len, (const char *[]){"fish must be set via UNDERTHEC_FISH, not here"}, 1);
        ok = false;
      } else if (!parse_fish_count(token + 5, &out->fish_count, errbuf, errbuf_len)) {
        ok = false;
      } else {
        *fish_set = true;
      }
    } else if (!aquatic_life_set_flag(out, token)) {
      set_errbuf(errbuf, errbuf_len, (const char *[]){"unknown aquatic-life entry '", token, "'"}, 3);
      ok = false;
    }
    token = buf + i + 1;
  }
  free(buf);
  return ok;
}

static int err_env_bad(const char *prog, const char *name, const char *errbuf) {
  write_parts(stderr, (const char *[]){prog, ": ", errbuf, " for ", name, "\n"}, 6);
  return 2;
}

static bool parse_bool_env(const char *val, bool *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "0") == 0) {
    *out = false;
    return true;
  }
  if (strcmp(val, "1") == 0) {
    *out = true;
    return true;
  }
  set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 0 or 1"}, 3);
  return false;
}

static bool parse_classic_env(const char *val, int *out_ver, char *errbuf, size_t errbuf_len) {
  if (val[0] == '\0' || strcmp(val, "1.0") == 0) {
    *out_ver = 1;
    return true;
  }
  if (strcmp(val, "1.1") == 0) {
    *out_ver = 2;
    return true;
  }
  set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 1.0 or 1.1"}, 3);
  return false;
}

static bool parse_pace(const char *s, double *out, char *errbuf, size_t errbuf_len) {
  size_t dot_count = 0;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == '.') {
      dot_count++;
      if (dot_count > 1) {
        set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
        return false;
      }
    } else if (*p < '0' || *p > '9') {
      set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
      return false;
    }
  }
  const char *dot = strchr(s, '.');
  if (dot != NULL && strlen(dot + 1) > 2) {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' has more than 2 decimal digits"}, 3);
    return false;
  }
  char *endptr = NULL;
  double val = strtod(s, &endptr);
  if (s[0] == '\0' || *endptr != '\0') {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
    return false;
  }
  if (val < 0.01 - 1e-9 || val > 10.0 + 1e-9) {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' out of range 0.01-10"}, 3);
    return false;
  }
  *out = val;
  return true;
}

static char *read_all_stdin(void) {
  size_t cap = 4096;
  size_t len = 0;
  char *buf = xmalloc(cap);
  size_t n;
  while ((n = fread(buf + len, 1, cap - len, stdin)) > 0) {
    len += n;
    if (len == cap) {
      cap *= 2;
      buf = xrealloc(buf, cap);
    }
  }
  buf[len] = '\0';
  return buf;
}

static int split_and_trim_lines(char *buf, char ***out_rows) {
  size_t cap = 16;
  char **rows = xmalloc(cap * sizeof(*rows));
  int count = 0;
  char *start = buf;
  for (char *p = buf;; p++) {
    if (*p == '\n' || *p == '\0') {
      char end = *p;
      *p = '\0';
      if ((size_t)count == cap) {
        cap *= 2;
        rows = xrealloc(rows, cap * sizeof(*rows));
      }
      rows[count++] = start;
      if (end == '\0') break;
      start = p + 1;
    }
  }
  while (count > 0 && rows[count - 1][0] == '\0') count--;
  *out_rows = rows;
  return count;
}

int main(int argc, char **argv) {
  bool c_given = false;
  bool a_given = false;
  bool t_given = false;
  bool s_given = false;
  bool p_given = false;
  bool fish_given = false;
  int classic_ver = 0; /* 0=off, 1=1.0, 2=1.1 */
  bool screensaver = false;
  bool transparent = false;
  double pace = 1.0;
  const char *message_arg = NULL;
  const char *message_color_arg = NULL;
  struct aquatic_life aquatic = aquatic_life_default();
  int i = 1;
  while (i < argc) {
    const char *a = argv[i];
    if (strcmp(a, "-c") == 0 || strcmp(a, "--classic") == 0) {
      c_given = true;
      classic_ver = 1;
      i++;
      if (i < argc && strcmp(argv[i], "1.1") == 0) {
        classic_ver = 2;
        i++;
      } else if (i < argc && strcmp(argv[i], "1.0") == 0) {
        i++;
      }
    } else if (strcmp(a, "-s") == 0 || strcmp(a, "--screensaver") == 0) {
      screensaver = true;
      s_given = true;
      i++;
    } else if (strcmp(a, "-t") == 0 || strcmp(a, "--transparent") == 0) {
      transparent = true;
      t_given = true;
      i++;
    } else if (strcmp(a, "-p") == 0 || strcmp(a, "--pace") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      char errbuf[128];
      if (!parse_pace(argv[i + 1], &pace, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        return 2;
      }
      p_given = true;
      i += 2;
    } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      print_help(argv[0]);
      return 0;
    } else if (strcmp(a, "-v") == 0 || strcmp(a, "--version") == 0) {
      write_parts(stdout, (const char *[]){
        TOOL_NAME, " v", TOOL_VERSION, "\n",
        "License GPLv2: GNU GPL version 2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n",
        "This is free software; you are free to change and redistribute it.\n",
        "There is NO WARRANTY, to the extent permitted by law.\n"
      }, 7);
      return 0;
    } else if (strcmp(a, "-m") == 0 || strcmp(a, "--message") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      message_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "-M") == 0 || strcmp(a, "--message-color") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      if (!color_name_valid(argv[i + 1])) {
        write_parts(stderr, (const char *[]){argv[0], ": invalid color '", argv[i + 1], "' for ", a, "\n"}, 6);
        return 2;
      }
      message_color_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "-a") == 0 || strcmp(a, "--aquatic-life") == 0) {
      a_given = true;
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      char errbuf[128];
      bool fish_set = false;
      if (!aquatic_life_parse(argv[i + 1], &aquatic, true, &fish_set, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        return 2;
      }
      if (fish_set) fish_given = true;
      i += 2;
    } else {
      write_parts(stderr, (const char *[]){argv[0], ": unknown option '", a, "'\n"}, 4);
      print_help(argv[0]);
      return 2;
    }
  }

  bool c_flag = c_given;
  bool a_flag = a_given;
  if (!t_given) {
    const char *env_val = getenv("UNDERTHEC_TRANSPARENT");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_bool_env(env_val, &transparent, errbuf, sizeof errbuf)) {
        return err_env_bad(argv[0], "UNDERTHEC_TRANSPARENT", errbuf);
      }
    }
  }
  if (!s_given) {
    const char *env_val = getenv("UNDERTHEC_SCREENSAVER");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_bool_env(env_val, &screensaver, errbuf, sizeof errbuf)) {
        return err_env_bad(argv[0], "UNDERTHEC_SCREENSAVER", errbuf);
      }
    }
  }
  if (!p_given) {
    const char *env_val = getenv("UNDERTHEC_PACE");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_pace(env_val, &pace, errbuf, sizeof errbuf)) {
        return err_env_bad(argv[0], "UNDERTHEC_PACE", errbuf);
      }
    }
  }
  if (message_arg == NULL) {
    const char *env_val = getenv("UNDERTHEC_MESSAGE");
    if (env_val != NULL) message_arg = env_val;
  }
  if (message_color_arg == NULL) {
    const char *env_val = getenv("UNDERTHEC_MESSAGE_COLOR");
    if (env_val != NULL) {
      if (!color_name_valid(env_val)) {
        char errbuf[128];
        set_errbuf(errbuf, sizeof errbuf, (const char *[]){"invalid color '", env_val, "'"}, 3);
        return err_env_bad(argv[0], "UNDERTHEC_MESSAGE_COLOR", errbuf);
      }
      message_color_arg = env_val;
    }
  }
  if (!fish_given) {
    const char *env_val = getenv("UNDERTHEC_FISH");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_fish_count(env_val, &aquatic.fish_count, errbuf, sizeof errbuf)) {
        return err_env_bad(argv[0], "UNDERTHEC_FISH", errbuf);
      }
      a_flag = true;
    }
  }
  if (!a_given) {
    const char *env_val = getenv("UNDERTHEC_AQUATIC_LIFE");
    if (env_val != NULL) {
      char errbuf[128];
      bool fish_set = false;
      if (!aquatic_life_parse(env_val, &aquatic, false, &fish_set, errbuf, sizeof errbuf)) {
        return err_env_bad(argv[0], "UNDERTHEC_AQUATIC_LIFE", errbuf);
      }
      a_flag = true;
    }
  }
  if (!c_given) {
    const char *env_val = getenv("UNDERTHEC_CLASSIC");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_classic_env(env_val, &classic_ver, errbuf, sizeof errbuf)) {
        return err_env_bad(argv[0], "UNDERTHEC_CLASSIC", errbuf);
      }
      c_flag = true;
    }
  }

  if (c_flag && a_flag) {
    write_parts(stderr, (const char *[]){argv[0], ": -c/--classic and -a/--aquatic-life are mutually exclusive\n"}, 2);
    return 2;
  }
  bool classic = (classic_ver == 1);
  if (classic_ver == 2) {
    aquatic = (struct aquatic_life){
        .fish_count = -1,
        .ship = true,
        .whale = true,
        .monster = true,
        .bigfish = true,
        .shark = true,
    };
  }
  char *message_buf = NULL;
  char **message_rows = NULL;
  int message_row_count = 0;
  if (message_arg != NULL) {
    message_buf = (strcmp(message_arg, "-") == 0) ? read_all_stdin() : owned_copy(message_arg);
    message_row_count = split_and_trim_lines(message_buf, &message_rows);
  }
  rng_seed((uint64_t)time(NULL) ^ ((uint64_t)clock() << 32));
  if (term_init() != 0) {
    write_parts(stderr, (const char *[]){argv[0], ": failed to initialize the terminal\n"}, 2);
    free(message_rows);
    free(message_buf);
    return 1;
  }
  term_set_transparent(transparent);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
  struct scene scene;
  scene_init(&scene, classic, aquatic);
  if (message_color_arg != NULL) scene_set_message_color(&scene, color_from_name(message_color_arg));
  if (message_row_count > 0) scene_set_message(&scene, (const char *const *)message_rows, message_row_count);
  free(message_rows);
  free(message_buf);
  struct canvas canvas;
  canvas_init(&canvas);
  int last_w = -1;
  int last_h = -1;
  bool paused = false;
  double tick_accum = 0.0;
  while (!g_should_quit) {
    int w;
    int h;
    term_size(&w, &h);
    if (w != last_w || h != last_h) {
      canvas_resize(&canvas, w, h);
      scene_reset(&scene, w, h);
      last_w = w;
      last_h = h;
    }
    int key = term_poll_key(100);
    if (key == 'q') break;
    if (screensaver && key != -1) break;
    if (key == 'r') scene_reset(&scene, w, h);
    if (key == 'p') paused = !paused;
    if (key == 't') {
      transparent = !transparent;
      term_set_transparent(transparent);
    }
    if (!paused) {
      tick_accum += pace;
      while (tick_accum >= 1.0) {
        scene_tick(&scene, w, h);
        tick_accum -= 1.0;
      }
    }
    canvas_clear(&canvas);
    scene_draw(&scene, &canvas);
    term_present(&canvas);
  }
  canvas_free(&canvas);
  scene_free(&scene);
  term_shutdown();
  return 0;
}
