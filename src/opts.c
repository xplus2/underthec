#include "opts.h"
#include "xalloc.h"

#include <stdlib.h>
#include <string.h>

void opts_append_bounded(char *dst, size_t dst_cap, size_t *pos, const char *src) {
  size_t src_len = strlen(src);
  size_t avail = dst_cap > *pos ? dst_cap - *pos : 0;
  if (src_len > avail) src_len = avail;
  memcpy(dst + *pos, src, src_len);
  *pos += src_len;
}

void opts_set_errbuf(char *errbuf, size_t errbuf_len, const char *const *parts, size_t count) {
  if (errbuf_len == 0) return;
  size_t pos = 0;
  for (size_t i = 0; i < count; i++) opts_append_bounded(errbuf, errbuf_len - 1, &pos, parts[i]);
  errbuf[pos] = '\0';
}

char *opts_strdup(const char *s) {
  size_t len = strlen(s);
  char *p = xmalloc(len + 1);
  memcpy(p, s, len + 1);
  return p;
}

bool opts_parse_fish_count(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "auto") == 0) {
    *out = -1;
    return true;
  }
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 0 || n > 100000) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fish count '", val, "'"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

bool opts_parse_aquatic_life(const char *definition, struct aquatic_life *out, bool allow_fish, bool *fish_set, char *errbuf, size_t errbuf_len) {
  scene_aquatic_fill(out, false);
  char *buf = opts_strdup(definition);
  size_t len = strlen(buf);
  bool ok = true;
  const char *token = buf;
  for (size_t i = 0; i <= len && ok; i++) {
    if (buf[i] != ',' && buf[i] != '\0') continue;
    buf[i] = '\0';
    if (token[0] == '\0') {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"empty entry in aquatic-life definition"}, 1);
      ok = false;
    } else if (strncmp(token, "fish=", 5) == 0) {
      if (!allow_fish) {
        opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"fish must be set via UNDERTHEC_FISH, not here"}, 1);
        ok = false;
      } else if (!opts_parse_fish_count(token + 5, &out->fish_count, errbuf, errbuf_len)) {
        ok = false;
      } else {
        *fish_set = true;
      }
    } else if (!scene_aquatic_set_flag(out, token)) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"unknown aquatic-life entry '", token, "'"}, 3);
      ok = false;
    }
    token = buf + i + 1;
  }
  free(buf);
  return ok;
}

bool opts_parse_classic(const char *val, int *out_ver, char *errbuf, size_t errbuf_len) {
  if (val[0] == '\0' || strcmp(val, "1.0") == 0) {
    *out_ver = 1;
    return true;
  }
  if (strcmp(val, "1.1") == 0) {
    *out_ver = 2;
    return true;
  }
  opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid value '", val, "', expected 1.0 or 1.1"}, 3);
  return false;
}

bool opts_parse_message_position(const char *val, enum message_position *out, char *errbuf, size_t errbuf_len) {
  if (strcmp(val, "middle") == 0) *out = MSG_POS_MIDDLE;
  else if (strcmp(val, "center") == 0) *out = MSG_POS_CENTER;
  else if (strcmp(val, "marquee") == 0) *out = MSG_POS_MARQUEE;
  else if (strcmp(val, "swim") == 0) *out = MSG_POS_SWIM;
  else if (strcmp(val, "event") == 0) *out = MSG_POS_EVENT;
  else {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid message position '", val, "'"}, 3);
    return false;
  }
  return true;
}

bool opts_parse_uturn_chance(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 0 || n > 1000000) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid uturn chance '", val, "'"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

bool opts_parse_fps(const char *val, int *out, char *errbuf, size_t errbuf_len) {
  char *endptr = NULL;
  long n = strtol(val, &endptr, 10);
  if (val[0] == '\0' || *endptr != '\0' || n < 1 || n > 120) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid fps '", val, "', expected 1-120"}, 3);
    return false;
  }
  *out = (int)n;
  return true;
}

bool opts_parse_pace(const char *s, double *out, char *errbuf, size_t errbuf_len) {
  size_t dot_count = 0;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == '.') {
      dot_count++;
      if (dot_count > 1) {
        opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
        return false;
      }
    } else if (*p < '0' || *p > '9') {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
      return false;
    }
  }
  const char *dot = strchr(s, '.');
  if (dot != NULL && strlen(dot + 1) > 2) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' has more than 2 decimal digits"}, 3);
    return false;
  }
  char *endptr = NULL;
  double val = strtod(s, &endptr);
  if (s[0] == '\0' || *endptr != '\0') {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"invalid pace '", s, "'"}, 3);
    return false;
  }
  if (val < 0.01 - 1e-9 || val > 10.0 + 1e-9) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"pace '", s, "' out of range 0.01-10"}, 3);
    return false;
  }
  *out = val;
  return true;
}

bool opts_parse_teletext_caption(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len) {
  size_t len = strlen(val);
  if (len >= out_cap) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"teletext caption '", val, "' too long, max 32 chars"}, 3);
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if ((unsigned char)val[i] < 0x20 || (unsigned char)val[i] > 0x7E) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"teletext caption '", val, "' must be printable ASCII"}, 3);
      return false;
    }
  }
  memcpy(out, val, len + 1);
  return true;
}

bool opts_parse_castle_name(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len) {
  size_t len = strlen(val);
  if (len >= out_cap) {
    opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"castle name '", val, "' too long, max 11 chars"}, 3);
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if ((unsigned char)val[i] < 0x20 || (unsigned char)val[i] > 0x7E) {
      opts_set_errbuf(errbuf, errbuf_len, (const char *[]){"castle name '", val, "' must be printable ASCII"}, 3);
      return false;
    }
  }
  memcpy(out, val, len + 1);
  return true;
}

int opts_split_lines(char *buf, char ***out_rows) {
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
