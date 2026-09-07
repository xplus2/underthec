#include "canvas.h"
#include "rng.h"
#include "scene.h"
#include "term/term.h"
#include "version.h"

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
  printf("  -h, --help            show this help\n");
  printf("  -v, --version         show version\n\n");
  printf("keys while running: q quit, r redraw, p pause\n");
}

static char *owned_copy(const char *s) {
  size_t len = strlen(s);
  char *p = malloc(len + 1);
  memcpy(p, s, len + 1);
  return p;
}

static char *read_all_stdin(void) {
  size_t cap = 4096, len = 0;
  char *buf = malloc(cap);
  size_t n;
  while ((n = fread(buf + len, 1, cap - len, stdin)) > 0) {
    len += n;
    if (len == cap) {
      cap *= 2;
      buf = realloc(buf, cap);
    }
  }
  buf[len] = '\0';
  return buf;
}

static int split_and_trim_lines(char *buf, char ***out_rows) {
  int n = 1;
  for (char *p = buf; *p != '\0'; p++) if (*p == '\n') n++;
  char **rows = malloc((size_t)n * sizeof(*rows));
  int count = 0;
  char *start = buf;
  for (char *p = buf;; p++) {
    if (*p == '\n' || *p == '\0') {
      char end = *p;
      *p = '\0';
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
  const char *message_arg = NULL;
  for (int i = 1; i < argc; i++) {
    const char *a = argv[i];
    if (strcmp(a, "-c") == 0 || strcmp(a, "--classic") == 0) {
      classic = true;
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
      message_arg = argv[++i];
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
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
  struct scene scene;
  scene_init(&scene, classic);
  if (message_row_count > 0) scene_set_message(&scene, (const char *const *)message_rows, message_row_count);
  free(message_rows);
  free(message_buf);
  struct canvas canvas;
  canvas_init(&canvas);
  int last_w = -1, last_h = -1;
  bool paused = false;
  while (!g_should_quit) {
    int w, h;
    term_size(&w, &h);
    if (w != last_w || h != last_h) {
      canvas_resize(&canvas, w, h);
      scene_reset(&scene, w, h);
      last_w = w;
      last_h = h;
    }

    int key = term_poll_key(100);
    if (key == 'q') break;
    if (key == 'r') scene_reset(&scene, w, h);
    if (key == 'p') paused = !paused;
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
