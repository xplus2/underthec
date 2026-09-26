#include "args.h"
#include "cli_help.h"

#include "../color.h"
#include "../opts.h"
#include "../version.h"

#include <stdlib.h>
#include <string.h>

void write_parts(FILE *stream, const char *const *parts, size_t count) {
  char buf[512] = {0};
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) opts_append_bounded(buf, sizeof(buf), &pos, parts[i]);
  fwrite(buf, 1, pos, stream);
}

static int err_requires_arg(const char *prog, const char *opt) {
  write_parts(stderr, (const char *[]){prog, ": ", opt, " requires an argument\n"}, 4);
  return 2;
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
  opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 0 or 1"}, 3);
  return false;
}

static bool parse_teletext_mode(const char *val, enum tt_mode *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "t42") == 0) *out = TT_T42;
  else if (strcmp(val, "ts") == 0) *out = TT_TS;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid teletext format '", val, "', expected t42 or ts"}, 3);
    return false;
  }
  return true;
}

static bool parse_teletext_glyphs(const char *val, enum tt_glyphs *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "text") == 0) *out = TT_TEXT;
  else if (strcmp(val, "mosaic") == 0) *out = TT_MOSAIC;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid teletext mode '", val, "', expected text or mosaic"}, 3);
    return false;
  }
  return true;
}

static bool parse_ttl(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 1 || n > 255) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid ttl '", val, "', expected 1-255"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

bool args_parse(int argc, char **argv, struct cli_args *out, int *exit_code) {
  bool c_given = false;
  bool a_given = false;
  bool t_given = false;
  bool s_given = false;
  bool p_given = false;
  bool fish_given = false;
  int classic_ver = 0; /* 0=off, 1=1.0, 2=1.1 */
  bool message_position_given = false;
  bool u_given = false;
  bool f_given = false;
  bool glyphs_given = false;
  bool ttl_given = false;
  bool teletext_caption_given = false;
  bool castle_name_given = false;
  bool no_castle_given = false;
  const char *teletext_arg = NULL;
  const char *glyphs_arg = NULL;

  out->screensaver = false;
  out->transparent = false;
  out->no_castle = false;
  out->pace = 1.0;
  out->fps = 10;
  out->uturn_chance = 200;
  out->message_arg = NULL;
  out->message_color_arg = NULL;
  out->message_position = MSG_POS_MIDDLE;
  out->mcast_arg = NULL;
  out->iface_arg = NULL;
  out->mcast_ttl = 1;
  snprintf(out->teletext_caption, sizeof(out->teletext_caption), "%s", TT_TITLE);
  memset(out->castle_name, 0, sizeof(out->castle_name));
  out->aquatic = scene_aquatic_default();

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
      out->screensaver = true;
      s_given = true;
      i++;
    } else if (strcmp(a, "-t") == 0 || strcmp(a, "--transparent") == 0) {
      out->transparent = true;
      t_given = true;
      i++;
    } else if (strcmp(a, "-p") == 0 || strcmp(a, "--pace") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!opts_parse_pace(argv[i + 1], &out->pace, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      p_given = true;
      i += 2;
    } else if (strcmp(a, "-u") == 0 || strcmp(a, "--uturn-chance") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!opts_parse_uturn_chance(argv[i + 1], &out->uturn_chance, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      u_given = true;
      i += 2;
    } else if (strcmp(a, "-f") == 0 || strcmp(a, "--fps") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!opts_parse_fps(argv[i + 1], &out->fps, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      f_given = true;
      i += 2;
    } else if (strncmp(a, "--teletext=", 11) == 0) {
      teletext_arg = a + 11;
      i++;
    } else if (strcmp(a, "--teletext") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      teletext_arg = argv[i + 1];
      i += 2;
    } else if (strncmp(a, "--teletext-mode=", 16) == 0) {
      glyphs_arg = a + 16;
      glyphs_given = true;
      i++;
    } else if (strcmp(a, "--teletext-mode") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      glyphs_arg = argv[i + 1];
      glyphs_given = true;
      i += 2;
    } else if (strcmp(a, "--mcast") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      out->mcast_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "--iface") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      out->iface_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "--ttl") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!parse_ttl(argv[i + 1], &out->mcast_ttl, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      ttl_given = true;
      i += 2;
    } else if (strcmp(a, "--teletext-caption") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!opts_parse_teletext_caption(argv[i + 1], out->teletext_caption, sizeof out->teletext_caption, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      teletext_caption_given = true;
      i += 2;
    } else if (strcmp(a, "--no-castle") == 0) {
      out->no_castle = true;
      no_castle_given = true;
      i++;
    } else if (strcmp(a, "-n") == 0 || strcmp(a, "--castle-name") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!opts_parse_castle_name(argv[i + 1], out->castle_name, sizeof out->castle_name, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      castle_name_given = true;
      i += 2;
    } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      print_help(argv[0]);
      *exit_code = 0;
      return false;
    } else if (strcmp(a, "-v") == 0 || strcmp(a, "--version") == 0) {
      write_parts(stdout, (const char *[]){
        TOOL_NAME, " v", TOOL_VERSION, "\n",
        "License GPLv2: GNU GPL version 2 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n",
        "This is free software; you are free to change and redistribute it.\n",
        "There is NO WARRANTY, to the extent permitted by law.\n"
      }, 7);
      *exit_code = 0;
      return false;
    } else if (strcmp(a, "-m") == 0 || strcmp(a, "--message") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      out->message_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "-M") == 0 || strcmp(a, "--message-color") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      if (!color_name_valid(argv[i + 1])) {
        write_parts(stderr, (const char *[]){argv[0], ": invalid color '", argv[i + 1], "' for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      out->message_color_arg = argv[i + 1];
      i += 2;
    } else if (strcmp(a, "-P") == 0 || strcmp(a, "--message-position") == 0) {
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      if (!opts_parse_message_position(argv[i + 1], &out->message_position, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      message_position_given = true;
      i += 2;
    } else if (strcmp(a, "-a") == 0 || strcmp(a, "--aquatic-life") == 0) {
      a_given = true;
      if (i + 1 >= argc) { *exit_code = err_requires_arg(argv[0], a); return false; }
      char errbuf[128];
      bool fish_set = false;
      if (!opts_parse_aquatic_life(argv[i + 1], &out->aquatic, true, &fish_set, errbuf, sizeof errbuf)) {
        write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, " for ", a, "\n"}, 6);
        *exit_code = 2;
        return false;
      }
      if (fish_set) fish_given = true;
      i += 2;
    } else {
      write_parts(stderr, (const char *[]){argv[0], ": unknown option '", a, "'\n"}, 4);
      print_help(argv[0]);
      *exit_code = 2;
      return false;
    }
  }

  bool c_flag = c_given;
  bool a_flag = a_given;
  if (!t_given) {
    const char *env_val = getenv("UNDERTHEC_TRANSPARENT");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_bool_env(env_val, &out->transparent, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_TRANSPARENT", errbuf);
        return false;
      }
    }
  }
  if (!s_given) {
    const char *env_val = getenv("UNDERTHEC_SCREENSAVER");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_bool_env(env_val, &out->screensaver, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_SCREENSAVER", errbuf);
        return false;
      }
    }
  }
  if (!p_given) {
    const char *env_val = getenv("UNDERTHEC_PACE");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_pace(env_val, &out->pace, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_PACE", errbuf);
        return false;
      }
    }
  }
  if (!f_given) {
    const char *env_val = getenv("UNDERTHEC_FPS");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_fps(env_val, &out->fps, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_FPS", errbuf);
        return false;
      }
    }
  }
  if (!u_given) {
    const char *env_val = getenv("UNDERTHEC_UTURN_CHANCE");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_uturn_chance(env_val, &out->uturn_chance, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_UTURN_CHANCE", errbuf);
        return false;
      }
    }
  }
  if (teletext_arg == NULL) teletext_arg = getenv("UNDERTHEC_TELETEXT");
  if (glyphs_arg == NULL) glyphs_arg = getenv("UNDERTHEC_TELETEXT_MODE");
  if (out->mcast_arg == NULL) out->mcast_arg = getenv("UNDERTHEC_MCAST");
  if (out->iface_arg == NULL) out->iface_arg = getenv("UNDERTHEC_MCAST_IFACE");
  if (!ttl_given) {
    const char *env_val = getenv("UNDERTHEC_MCAST_TTL");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_ttl(env_val, &out->mcast_ttl, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_MCAST_TTL", errbuf);
        return false;
      }
    }
  }
  if (!teletext_caption_given) {
    const char *env_val = getenv("UNDERTHEC_TELETEXT_CAPTION");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_teletext_caption(env_val, out->teletext_caption, sizeof out->teletext_caption, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_TELETEXT_CAPTION", errbuf);
        return false;
      }
    }
  }
  if (!no_castle_given) {
    const char *env_val = getenv("UNDERTHEC_NO_CASTLE");
    if (env_val != NULL) {
      char errbuf[128];
      if (!parse_bool_env(env_val, &out->no_castle, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_NO_CASTLE", errbuf);
        return false;
      }
    }
  }
  if (!castle_name_given) {
    const char *env_val = getenv("UNDERTHEC_CASTLE_NAME");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_castle_name(env_val, out->castle_name, sizeof out->castle_name, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_CASTLE_NAME", errbuf);
        return false;
      }
    }
  }
  if (out->message_arg == NULL) {
    const char *env_val = getenv("UNDERTHEC_MESSAGE");
    if (env_val != NULL) out->message_arg = env_val;
  }
  if (out->message_color_arg == NULL) {
    const char *env_val = getenv("UNDERTHEC_MESSAGE_COLOR");
    if (env_val != NULL) {
      if (!color_name_valid(env_val)) {
        char errbuf[128];
        opts_set_errbuf(errbuf, sizeof errbuf, (const char *[]){"invalid color '", env_val, "'"}, 3);
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_MESSAGE_COLOR", errbuf);
        return false;
      }
      out->message_color_arg = env_val;
    }
  }
  if (!message_position_given) {
    const char *env_val = getenv("UNDERTHEC_MESSAGE_POSITION");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_message_position(env_val, &out->message_position, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_MESSAGE_POSITION", errbuf);
        return false;
      }
    }
  }
  if (!fish_given) {
    const char *env_val = getenv("UNDERTHEC_FISH");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_fish_count(env_val, &out->aquatic.fish_count, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_FISH", errbuf);
        return false;
      }
      a_flag = true;
    }
  }
  if (!a_given) {
    const char *env_val = getenv("UNDERTHEC_AQUATIC_LIFE");
    if (env_val != NULL) {
      char errbuf[128];
      bool fish_set = false;
      if (!opts_parse_aquatic_life(env_val, &out->aquatic, false, &fish_set, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_AQUATIC_LIFE", errbuf);
        return false;
      }
      a_flag = true;
    }
  }
  if (!c_given) {
    const char *env_val = getenv("UNDERTHEC_CLASSIC");
    if (env_val != NULL) {
      char errbuf[128];
      if (!opts_parse_classic(env_val, &classic_ver, errbuf, sizeof errbuf)) {
        *exit_code = err_env_bad(argv[0], "UNDERTHEC_CLASSIC", errbuf);
        return false;
      }
      c_flag = true;
    }
  }

  if (c_flag && a_flag) {
    write_parts(stderr, (const char *[]){argv[0], ": -c/--classic and -a/--aquatic-life are mutually exclusive\n"}, 2);
    *exit_code = 2;
    return false;
  }
  out->classic = (classic_ver == 1);
  if (classic_ver == 2) out->aquatic = scene_aquatic_classic11();

  out->teletext_requested = (teletext_arg != NULL);
  out->mcast_requested = (out->mcast_arg != NULL);
  out->tt_mode = TT_TS;
  out->tt_glyphs = TT_TEXT;
  if (glyphs_given && !out->teletext_requested && !out->mcast_requested) {
    write_parts(stderr, (const char *[]){argv[0], ": --teletext-mode requires --teletext or --mcast\n"}, 2);
    *exit_code = 2;
    return false;
  }
  if (out->teletext_requested || out->mcast_requested) {
    char errbuf[128];
    if (out->teletext_requested && !parse_teletext_mode(teletext_arg, &out->tt_mode, errbuf, sizeof errbuf)) {
      write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
      *exit_code = 2;
      return false;
    }
    if (glyphs_arg != NULL && !parse_teletext_glyphs(glyphs_arg, &out->tt_glyphs, errbuf, sizeof errbuf)) {
      write_parts(stderr, (const char *[]){argv[0], ": ", errbuf, "\n"}, 4);
      *exit_code = 2;
      return false;
    }
    if (out->mcast_requested && out->tt_mode == TT_T42) {
      write_parts(stderr, (const char *[]){argv[0], ": --mcast requires the ts format\n"}, 2);
      *exit_code = 2;
      return false;
    }
  }

  *exit_code = 0;
  return true;
}
