#include "canvas.h"
#include "color.h"
#include "rng.h"
#include "scene.h"
#include "term/term.h"
#include "version.h"
#include "xalloc.h"

#include <signal.h>
#include <stdbool.h>
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

static void print_help(const char *prog) {
  printf("usage: %s [options]\n\n", prog);
  printf("  -c, --classic         classic mode (asciiquarium 1.0)\n");
  printf("  -m, --message <text>  show background text/ascii art\n");
  printf("                        ('-' reads it from stdin)\n");
  printf("  -M, --message-color <color>\n");
  printf("                        -m text color (default: blue)\n");
  printf("                        red, green, blue, yellow, magenta, cyan, white, black\n");
  printf("                        capitalized first letter=bold\n");
  printf("  -a, --aquatic-life <definition>\n");
  printf("                        comma-separated, default: all on, fish=auto\n");
  printf("                        fish=<N|auto>,ducks,dolphins,ship,swan,kaiju,crab,shark,\n");
  printf("                        submarine,whale,jellyfish,monster,bigfish,swordfish\n");
  printf("  -s, --screensaver     exit on any keypress\n");
  printf("  -t, --transparent     transparent background (default: opaque black)\n");
  printf("  -h, --help            show this help\n");
  printf("  -v, --version         show version\n\n");
  printf("keys while running: q quit, r redraw, p pause, t toggle transparency\n");
}

static char *owned_copy(const char *s) {
  size_t len = strlen(s);
  char *p = xmalloc(len + 1);
  memcpy(p, s, len + 1);
  return p;
}

static struct aquatic_life aquatic_life_default(void) {
  struct aquatic_life a;
  a.fish_count = -1;
  a.ducks = true;
  a.dolphins = true;
  a.ship = true;
  a.swan = true;
  a.kaiju = true;
  a.fishhook = true;
  a.submarine = true;
  a.whale = true;
  a.shark = true;
  a.jellyfish = true;
  a.monster = true;
  a.bigfish = true;
  a.swordfish = true;
  a.crab = true;
  return a;
}

static bool aquatic_life_set_flag(struct aquatic_life *out, const char *name) {
  if (strcmp(name, "ducks") == 0)          out->ducks = true;
  else if (strcmp(name, "dolphins") == 0)  out->dolphins = true;
  else if (strcmp(name, "ship") == 0)      out->ship = true;
  else if (strcmp(name, "swan") == 0)      out->swan = true;
  else if (strcmp(name, "kaiju") == 0)     out->kaiju = true;
  else if (strcmp(name, "fishhook") == 0)  out->fishhook = true;
  else if (strcmp(name, "submarine") == 0) out->submarine = true;
  else if (strcmp(name, "whale") == 0)     out->whale = true;
  else if (strcmp(name, "shark") == 0)     out->shark = true;
  else if (strcmp(name, "jellyfish") == 0) out->jellyfish = true;
  else if (strcmp(name, "monster") == 0)   out->monster = true;
  else if (strcmp(name, "bigfish") == 0)   out->bigfish = true;
  else if (strcmp(name, "swordfish") == 0) out->swordfish = true;
  else if (strcmp(name, "crab") == 0)      out->crab = true;
  else return false;
  return true;
}

static bool aquatic_life_parse(const char *definition, struct aquatic_life *out, char *errbuf, size_t errbuf_len) {
  out->fish_count = -1;
  out->ducks = false;
  out->dolphins = false;
  out->ship = false;
  out->swan = false;
  out->kaiju = false;
  out->fishhook = false;
  out->submarine = false;
  out->whale = false;
  out->shark = false;
  out->jellyfish = false;
  out->monster = false;
  out->bigfish = false;
  out->swordfish = false;
  out->crab = false;

  char *buf = owned_copy(definition);
  size_t len = strlen(buf);
  bool ok = true;
  char *token = buf;
  for (size_t i = 0; i <= len && ok; i++) {
    if (buf[i] != ',' && buf[i] != '\0') continue;
    buf[i] = '\0';
    if (token[0] == '\0') {
      snprintf(errbuf, errbuf_len, "empty entry in aquatic-life definition");
      ok = false;
    } else if (strncmp(token, "fish=", 5) == 0) {
      const char *val = token + 5;
      if (strcmp(val, "auto") == 0) {
        out->fish_count = -1;
      } else {
        char *endptr = NULL;
        long n = strtol(val, &endptr, 10);
        if (val[0] == '\0' || *endptr != '\0' || n < 0 || n > 100000) {
          snprintf(errbuf, errbuf_len, "invalid fish count '%s'", val);
          ok = false;
        } else out->fish_count = (int)n;
      }
    } else if (!aquatic_life_set_flag(out, token)) {
      snprintf(errbuf, errbuf_len, "unknown aquatic-life entry '%s'", token);
      ok = false;
    }
    token = buf + i + 1;
  }
  free(buf);
  return ok;
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
  bool classic = false;
  bool screensaver = false;
  bool transparent = false;
  const char *message_arg = NULL;
  const char *message_color_arg = NULL;
  struct aquatic_life aquatic = aquatic_life_default();
  int i = 1;
  while (i < argc) {
    const char *a = argv[i];
    if (strcmp(a, "-c") == 0 || strcmp(a, "--classic") == 0) {
      classic = true;
      i++;
    } else if (strcmp(a, "-s") == 0 || strcmp(a, "--screensaver") == 0) {
      screensaver = true;
      i++;
    } else if (strcmp(a, "-t") == 0 || strcmp(a, "--transparent") == 0) {
      transparent = true;
      i++;
    } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      print_help(argv[0]);
      return 0;
    } else if (strcmp(a, "-v") == 0 || strcmp(a, "--version") == 0) {
      printf("%s %s\n", TOOL_NAME, TOOL_VERSION);
      return 0;
    } else if (strcmp(a, "-m") == 0 || strcmp(a, "--message") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "%s: %s requires an argument\n", argv[0], a);
        return 2;
      }
      message_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "-M") == 0 || strcmp(a, "--message-color") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "%s: %s requires an argument\n", argv[0], a);
        return 2;
      }
      if (!color_name_valid(argv[i + 1])) {
        fprintf(stderr, "%s: invalid color '%s' for %s\n", argv[0], argv[i + 1], a);
        return 2;
      }
      message_color_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "-a") == 0 || strcmp(a, "--aquatic-life") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "%s: %s requires an argument\n", argv[0], a);
        return 2;
      }
      char errbuf[128];
      if (!aquatic_life_parse(argv[i + 1], &aquatic, errbuf, sizeof errbuf)) {
        fprintf(stderr, "%s: %s for %s\n", argv[0], errbuf, a);
        return 2;
      }
      i += 2;
    } else {
      fprintf(stderr, "%s: unknown option '%s'\n", argv[0], a);
      print_help(argv[0]);
      return 2;
    }
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
    fprintf(stderr, "%s: failed to initialize the terminal\n", argv[0]);
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
    if (!paused)    scene_tick(&scene, w, h);

    canvas_clear(&canvas);
    scene_draw(&scene, &canvas);
    term_present(&canvas);
  }

  canvas_free(&canvas);
  scene_free(&scene);
  term_shutdown();
  return 0;
}
