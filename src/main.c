#define _DEFAULT_SOURCE

#include "app.h"
#include "canvas.h"
#include "color.h"
#include "opts.h"
#include "rng.h"
#include "scene.h"
#include "target/teletext/teletext.h"
#include "target/terminal/term.h"
#include "ui/args.h"
#include "xalloc.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

static volatile sig_atomic_t g_should_quit = 0;
static volatile sig_atomic_t g_feed_signal = 0;

static void on_signal(int sig) {
  (void)sig;
  g_should_quit = 1;
}

#ifdef SIGUSR1
static void on_feed_signal(int sig) {
  (void)sig;
  g_feed_signal = 1;
}
#endif

static double now_seconds(void) {
#ifdef _WIN32
  LARGE_INTEGER freq, counter;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)freq.QuadPart;
#else
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
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

int main(int argc, char **argv) {
  struct cli_args args;
  int exit_code = 0;
  if (!args_parse(argc, argv, &args, &exit_code)) return exit_code;

  struct tt_stream *tt = NULL;
  struct tt_net *tt_net = NULL;
  if (args.teletext_requested || args.mcast_requested) {
    char errbuf[128];
    if (args.mcast_requested) {
      tt_net = tt_net_open(args.mcast_arg, args.mcast_ttl, args.iface_arg, errbuf, sizeof errbuf);
      if (tt_net == NULL) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
        return 2;
      }
    } else if (tt_stdout_is_tty()) {
      write_parts(stderr, (const char *[]){argv[0], ": refusing to write binary teletext to a terminal\n"}, 2);
      return 2;
    }
    tt = tt_stream_open(args.tt_mode, args.tt_glyphs, tt_net, args.fps, args.teletext_caption);
    if (tt == NULL) {
      write_parts(stderr, (const char *[]){argv[0], ": failed to open the teletext output\n"}, 2);
      return 1;
    }
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
  }

  char *message_buf = NULL;
  char **message_rows = NULL;
  int message_row_count = 0;
  if (args.message_arg != NULL) {
    message_buf = (strcmp(args.message_arg, "-") == 0) ? read_all_stdin() : opts_strdup(args.message_arg);
    message_row_count = opts_split_lines(message_buf, &message_rows);
  }
  rng_seed((uint64_t)time(NULL) ^ ((uint64_t)clock() << 32));
  if (tt == NULL && term_init() != 0) {
    write_parts(stderr, (const char *[]){argv[0], ": failed to initialize the terminal\n"}, 2);
    free(message_rows);
    free(message_buf);
    return 1;
  }
  bool transparent = args.transparent;
  if (tt == NULL) term_set_transparent(transparent);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);
#ifdef SIGUSR1
  struct sigaction feed_sa;
  memset(&feed_sa, 0, sizeof(feed_sa));
  feed_sa.sa_handler = on_feed_signal;
  feed_sa.sa_flags = SA_RESTART;
  sigaction(SIGUSR1, &feed_sa, NULL);
#endif
  struct app app;
  app_init(&app, args.classic, args.aquatic, args.pace, args.fps, now_seconds());
  scene_set_castle(&app.scene, !args.no_castle);
  if (args.castle_name[0] != '\0') scene_set_castle_name(&app.scene, args.castle_name);
  if (args.message_color_arg != NULL) scene_set_message_color(&app.scene, color_from_name(args.message_color_arg));
  scene_set_message_position(&app.scene, args.message_position);
  scene_set_uturn_chance(&app.scene, args.uturn_chance);
  if (message_row_count > 0) scene_set_message(&app.scene, (const char *const *)message_rows, message_row_count);
  free(message_rows);
  free(message_buf);
  exit_code = 0;
  double deadline = app.last;
  while (!g_should_quit) {
    int w;
    int h;
    if (tt != NULL) {
      w = tt_canvas_w(args.tt_glyphs);
      h = TT_CANVAS_H;
    } else {
      term_size(&w, &h);
    }
    app_resize(&app, w, h);
    double frame_period = 1.0 / (double)app.fps;
    deadline += frame_period;
    double wait = deadline - now_seconds();
    if (wait < -frame_period) deadline = now_seconds();
    int wait_ms = wait > 0.0 ? (int)(wait * 1000.0 + 0.999) : 0;
    int key = -1;
    if (tt != NULL) tt_sleep_ms(wait_ms);
    else key = term_poll_key(wait_ms);
    if (key == 'q') break;
    if (args.screensaver && key != -1) break;
    app_key(&app, key);
    if (key == 't') {
      transparent = !transparent;
      term_set_transparent(transparent);
    }
    if (g_feed_signal) {
      g_feed_signal = 0;
      app_feed(&app, FEED_COL_AUTO);
    }
    app_frame(&app, now_seconds());
    if (tt != NULL) {
      if (tt_stream_present(tt, &app.canvas) != 0) {
        if (tt_net != NULL) {
          write_parts(stderr, (const char *[]){argv[0], ": multicast send failed\n"}, 2);
          exit_code = 1;
        }
        break;
      }
    } else {
      term_present(&app.canvas);
    }
  }
  app_free(&app);
  if (tt != NULL) {
    tt_stream_close(tt);
    if (tt_net != NULL) tt_net_close(tt_net);
  } else {
    term_shutdown();
  }
  return exit_code;
}
