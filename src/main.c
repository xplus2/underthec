#define _DEFAULT_SOURCE

#include "canvas.h"
#include "color.h"
#include "help.h"
#include "rng.h"
#include "scene.h"
#include "settings.h"
#include "teletext/teletext.h"
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
static volatile sig_atomic_t g_feed_signal = 0;

static void on_signal(int sig) {
  (void)sig;
  g_should_quit = 1;
}

static void on_feed_signal(int sig) {
  (void)sig;
  g_feed_signal = 1;
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
    "                          comma-separated, default: all on, fish=auto\n"
    "                          fish=<N|auto>,ducks,dolphins,ship,swan,kaiju,crab,shark,\n"
    "                          submarine,whale,jellyfish,monster,bigfish,swordfish\n"
    "  -c, --classic [1.0|1.1] classic mode, no arg = 1.0\n"
    "  -m, --message <text>    bg text/ascii art ('-' for stdin)\n"
    "  -M, --message-color <color>\n"
    "                          -m text color (default: blue)\n"
    "                          red,green,blue,yellow,magenta,cyan,white,black\n"
    "                          capitalized first letter=bold\n"
    "  -P, --message-position [middle|center|marquee|swim|event]\n"
    "                          -m placement (default: middle)\n"
    "                          middle: horizontally+vertically centered\n"
    "                          center: horizontally centered, vertical top\n"
    "                          marquee: centered scrolling\n"
    "                          swim: top row, scrolls right to left\n"
    "                          event: like swim, but random\n"
    "  -p, --pace <pace>       speed multiplier, 0.01-10 (default: 1)\n"
    "  -u, --uturn-chance <N>  fish turn around once per N ticks on average\n"
    "                          (default: 200, 0 = never)\n"
    "  -f, --fps <N>           render frames per second, 1-120 (default: 10)\n"
    "  -s, --screensaver       exit on any keypress\n"
    "  -t, --transparent       transparent background (default: opaque black)\n"
    "      --teletext <t42|ts> binary teletext stream to stdout instead of the terminal\n"
    "                          t42: raw 42-byte packets, ts: MPEG-TS with teletext PES\n"
    "      --teletext-mode <text|mosaic>\n"
    "                          text: real characters, 39 columns, exact ASCII via X/26\n"
    "                          mosaic: 2x3 blocks, 78 columns (default: text)\n"
    "      --mcast <IP:PORT>   send MPEG-TS teletext to a multicast group, [GROUP]:PORT for IPv6\n"
    "      --ttl <N>           multicast TTL/hops, 1-255 (default: 1)\n"
    "      --iface <if>        multicast interface: local address (IPv4) or name (IPv6)\n"
    "  -h, --help              show this help\n"
    "  -v, --version           show version\n\n"
    "keys while running:\n"
    "  q quit, r redraw, p pause, t toggle transparency, f feed, s settings, h help\n\n"
    "environment variables:\n"
    "  UNDERTHEC_FISH=auto|number          like -a's fish=\n"
    "  UNDERTHEC_AQUATIC_LIFE=<def>        like -a, except for fish=\n"
    "  UNDERTHEC_CLASSIC=1.0|1.1           like -c\n"
    "  UNDERTHEC_MESSAGE=<text>            like -m\n"
    "  UNDERTHEC_MESSAGE_COLOR=<color>     like -M\n"
    "  UNDERTHEC_MESSAGE_POSITION=<pos>    like -P\n"
    "  UNDERTHEC_PACE=<pace>               like -p\n"
    "  UNDERTHEC_FPS=<N>                   like -f\n"
    "  UNDERTHEC_SCREENSAVER=0|1           like -s\n"
    "  UNDERTHEC_UTURN_CHANCE=<N>          like -u\n"
    "  UNDERTHEC_TRANSPARENT=0|1           like -t\n"
    "  UNDERTHEC_TELETEXT=t42|ts           like --teletext\n"
    "  UNDERTHEC_TELETEXT_MODE=text|mosaic like --teletext-mode\n"
    "  UNDERTHEC_MCAST=<GROUP:PORT>        like --mcast\n"
    "  UNDERTHEC_MCAST_TTL=<N>             like --ttl\n"
    "  UNDERTHEC_MCAST_IFACE=<if>          like --iface\n",
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

static bool parse_message_position(const char *val, enum message_position *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "middle") == 0) *out = MSG_POS_MIDDLE;
  else if (strcmp(val, "center") == 0) *out = MSG_POS_CENTER;
  else if (strcmp(val, "marquee") == 0) *out = MSG_POS_MARQUEE;
  else if (strcmp(val, "swim") == 0) *out = MSG_POS_SWIM;
  else if (strcmp(val, "event") == 0) *out = MSG_POS_EVENT;
  else {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid message position '", val, "'"}, 3);
    return false;
  }
  return true;
}

static bool parse_uturn_chance(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 0 || n > 1000000) {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid uturn chance '", val, "'"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

static bool parse_fps(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 1 || n > 120) {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fps '", val, "', expected 1-120"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

static bool parse_teletext_mode(const char *val, enum tt_mode *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "t42") == 0) *out = TT_T42;
  else if (strcmp(val, "ts") == 0) *out = TT_TS;
  else {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid teletext format '", val, "', expected t42 or ts"}, 3);
    return false;
  }
  return true;
}

static bool parse_teletext_glyphs(const char *val, enum tt_glyphs *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "text") == 0) *out = TT_TEXT;
  else if (strcmp(val, "mosaic") == 0) *out = TT_MOSAIC;
  else {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid teletext mode '", val, "', expected text or mosaic"}, 3);
    return false;
  }
  return true;
}

static bool parse_ttl(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 1 || n > 255) {
    set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid ttl '", val, "', expected 1-255"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

static double now_seconds(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
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
  bool message_position_given = false;
  bool u_given = false;
  bool f_given = false;
  int fps = 10;
  int uturn_chance = 200;
  const char *message_arg = NULL;
  const char *message_color_arg = NULL;
  const char *teletext_arg = NULL;
  const char *glyphs_arg = NULL;
  bool glyphs_given = false;
  const char *mcast_arg = NULL;
  const char *iface_arg = NULL;
  int mcast_ttl = 1;
  bool ttl_given = false;
  enum message_position message_position = MSG_POS_MIDDLE;
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
    } else if (strcmp(a, "-u") == 0 || strcmp(a, "--uturn-chance") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      char errbuf[128];
      if (!parse_uturn_chance(argv[i + 1], &uturn_chance, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        return 2;
      }
      u_given = true;
      i += 2;
    } else if (strcmp(a, "-f") == 0 || strcmp(a, "--fps") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      char errbuf[128];
      if (!parse_fps(argv[i + 1], &fps, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        return 2;
      }
      f_given = true;
      i += 2;
    } else if (strncmp(a, "--teletext=", 11) == 0) {
      teletext_arg = a + 11;
      i++;
    } else if (strcmp(a, "--teletext") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      teletext_arg = argv[i + 1];
      i += 2;
    } else if (strncmp(a, "--teletext-mode=", 16) == 0) {
      glyphs_arg = a + 16;
      glyphs_given = true;
      i++;
    } else if (strcmp(a, "--teletext-mode") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      glyphs_arg = argv[i + 1];
      glyphs_given = true;
      i += 2;
    } else if (strcmp(a, "--mcast") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      mcast_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "--iface") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      iface_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "--ttl") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      char errbuf[128];
      if (!parse_ttl(argv[i + 1], &mcast_ttl, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        return 2;
      }
      ttl_given = true;
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
    } else if (strcmp(a, "-P") == 0 || strcmp(a, "--message-position") == 0) {
      if (i + 1 >= argc) return err_requires_arg(argv[0], a);
      char errbuf[128];
      if (!parse_message_position(argv[i + 1], &message_position, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        return 2;
      }
      message_position_given = true;
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
      if (!parse_bool_env(env_val, &transparent, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_TRANSPARENT", errbuf);
    }
  }
  if (!s_given) {
    const char *env_val = getenv("UNDERTHEC_SCREENSAVER");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_bool_env(env_val, &screensaver, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_SCREENSAVER", errbuf);
    }
  }
  if (!p_given) {
    const char *env_val = getenv("UNDERTHEC_PACE");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_pace(env_val, &pace, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_PACE", errbuf);
    }
  }
  if (!f_given) {
    const char *env_val = getenv("UNDERTHEC_FPS");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_fps(env_val, &fps, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_FPS", errbuf);
    }
  }
  if (!u_given) {
    const char *env_val = getenv("UNDERTHEC_UTURN_CHANCE");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_uturn_chance(env_val, &uturn_chance, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_UTURN_CHANCE", errbuf);
    }
  }
  if (teletext_arg == NULL) teletext_arg = getenv("UNDERTHEC_TELETEXT");
  if (glyphs_arg == NULL) glyphs_arg = getenv("UNDERTHEC_TELETEXT_MODE");
  if (mcast_arg == NULL) mcast_arg = getenv("UNDERTHEC_MCAST");
  if (iface_arg == NULL) iface_arg = getenv("UNDERTHEC_MCAST_IFACE");
  if (!ttl_given) {
    const char *env_val = getenv("UNDERTHEC_MCAST_TTL");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_ttl(env_val, &mcast_ttl, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_MCAST_TTL", errbuf);
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
  if (!message_position_given) {
    const char *env_val = getenv("UNDERTHEC_MESSAGE_POSITION");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_message_position(env_val, &message_position, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_MESSAGE_POSITION", errbuf);
    }
  }
  if (!fish_given) {
    const char *env_val = getenv("UNDERTHEC_FISH");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_fish_count(env_val, &aquatic.fish_count, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_FISH", errbuf);
      a_flag = true;
    }
  }
  if (!a_given) {
    const char *env_val = getenv("UNDERTHEC_AQUATIC_LIFE");
    if (env_val != NULL) {
      char errbuf[128];
      bool fish_set = false;
      if (!aquatic_life_parse(env_val, &aquatic, false, &fish_set, errbuf, sizeof errbuf))
        return err_env_bad(argv[0], "UNDERTHEC_AQUATIC_LIFE", errbuf);
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
  struct tt_stream *tt = NULL;
  struct tt_net *tt_net = NULL;
  enum tt_glyphs tt_glyphs = TT_TEXT;
  if (glyphs_given && teletext_arg == NULL && mcast_arg == NULL) {
    write_parts(stderr, (const char *[]){argv[0], ": --teletext-mode requires --teletext or --mcast\n"}, 2);
    return 2;
  }
  if (teletext_arg != NULL || mcast_arg != NULL) {
    enum tt_mode tt_mode = TT_TS;
    char errbuf[128];
    if (teletext_arg != NULL && !parse_teletext_mode(teletext_arg, &tt_mode, errbuf, sizeof errbuf)) {
      write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
      return 2;
    }
    if (glyphs_arg != NULL && !parse_teletext_glyphs(glyphs_arg, &tt_glyphs, errbuf, sizeof errbuf)) {
      write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
      return 2;
    }
    if (mcast_arg != NULL) {
      if (tt_mode == TT_T42) {
        write_parts(stderr, (const char *[]){argv[0], ": --mcast requires the ts format\n"}, 2);
        return 2;
      }
      tt_net = tt_net_open(mcast_arg, mcast_ttl, iface_arg, errbuf, sizeof errbuf);
      if (tt_net == NULL) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
        return 2;
      }
    } else if (tt_stdout_is_tty()) {
      write_parts(stderr, (const char *[]){argv[0], ": refusing to write binary teletext to a terminal\n"}, 2);
      return 2;
    }
    tt = tt_stream_open(tt_mode, tt_glyphs, tt_net, fps);
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
  if (message_arg != NULL) {
    message_buf = (strcmp(message_arg, "-") == 0) ? read_all_stdin() : owned_copy(message_arg);
    message_row_count = split_and_trim_lines(message_buf, &message_rows);
  }
  rng_seed((uint64_t)time(NULL) ^ ((uint64_t)clock() << 32));
  if (tt == NULL && term_init() != 0) {
    write_parts(stderr, (const char *[]){argv[0], ": failed to initialize the terminal\n"}, 2);
    free(message_rows);
    free(message_buf);
    return 1;
  }
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
  struct scene scene;
  scene_init(&scene, classic, aquatic);
  if (message_color_arg != NULL) scene_set_message_color(&scene, color_from_name(message_color_arg));
  scene_set_message_position(&scene, message_position);
  scene_set_uturn_chance(&scene, uturn_chance);
  if (message_row_count > 0) scene_set_message(&scene, (const char *const *)message_rows, message_row_count);
  free(message_rows);
  free(message_buf);
  struct settings_ui settings_ui;
  settings_ui_init(&settings_ui, &fps, &pace, &scene);
  struct help_ui help_ui;
  help_ui_init(&help_ui, &fps, &pace);
  struct canvas canvas;
  canvas_init(&canvas);
  int last_w = -1;
  int last_h = -1;
  bool paused = false;
  int exit_code = 0;
  double tick_accum = 0.0;
  double last = now_seconds();
  double deadline = last;
  while (!g_should_quit) {
    int w;
    int h;
    if (tt != NULL) {
      w = tt_canvas_w(tt_glyphs);
      h = TT_CANVAS_H;
    } else {
      term_size(&w, &h);
    }
    if (w != last_w || h != last_h) {
      canvas_resize(&canvas, w, h);
      scene_reset(&scene, w, h);
      last_w = w;
      last_h = h;
    }
    double tick_hz = 10.0 * pace;
    double frame_period = 1.0 / (double)fps;
    deadline += frame_period;
    double wait = deadline - now_seconds();
    if (wait < -frame_period) deadline = now_seconds();
    int wait_ms = wait > 0.0 ? (int)(wait * 1000.0 + 0.999) : 0;
    int key = -1;
    if (tt != NULL) tt_sleep_ms(wait_ms);
    else key = term_poll_key(wait_ms);
    if (key == 'q') break;
    if (screensaver && key != -1) break;
    if (key == 'r') scene_reset(&scene, w, h);
    if (key == 'p') paused = !paused;
    if (key == 't') {
      transparent = !transparent;
      term_set_transparent(transparent);
    }
    if (key == 'f') scene_feed(&scene, w, h);
    if (key == 's') {
      if (help_ui_is_open(&help_ui)) help_ui_close(&help_ui);
      settings_ui_toggle(&settings_ui);
    }
    if (key == 'h') {
      if (settings_ui_is_open(&settings_ui)) settings_ui_close(&settings_ui);
      help_ui_toggle(&help_ui);
    }
    if (settings_ui_is_open(&settings_ui)) {
      if (key == '\x1b') settings_ui_close(&settings_ui);
      else settings_ui_handle_key(&settings_ui, key, w, h);
    }
    if (help_ui_is_open(&help_ui) && key == '\x1b') help_ui_close(&help_ui);
    if (g_feed_signal) {
      g_feed_signal = 0;
      scene_feed(&scene, w, h);
    }
    double now = now_seconds();
    double dt = now - last;
    last = now;
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.5) dt = 0.5;
    if (!paused) {
      tick_accum += dt * tick_hz;
      while (tick_accum >= 1.0) {
        scene_tick(&scene, w, h);
        tick_accum -= 1.0;
      }
    }
    canvas_clear(&canvas);
    scene_draw(&scene, &canvas, tick_accum);
    settings_ui_draw(&settings_ui, &canvas);
    help_ui_draw(&help_ui, &canvas);
    if (tt != NULL) {
      if (tt_stream_present(tt, &canvas) != 0) {
        if (tt_net != NULL) {
          write_parts(stderr, (const char *[]){argv[0], ": multicast send failed\n"}, 2);
          exit_code = 1;
        }
        break;
      }
    } else {
      term_present(&canvas);
    }
  }
  canvas_free(&canvas);
  scene_free(&scene);
  if (tt != NULL) {
    tt_stream_close(tt);
    if (tt_net != NULL) tt_net_close(tt_net);
  } else {
    term_shutdown();
  }
  return exit_code;
}
