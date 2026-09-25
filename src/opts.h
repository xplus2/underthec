#ifndef UNDERTHEC_OPTS_H
#define UNDERTHEC_OPTS_H

#include <stdbool.h>
#include <stddef.h>

#include "scene.h"

/* value parsers shared by CLI and web. false: errbuf set */

void opts_append_bounded(char *dst, size_t dst_cap, size_t *pos, const char *src);
void opts_set_errbuf(char *errbuf, size_t errbuf_len, const char *const *parts, size_t count);
char *opts_strdup(const char *s);

/* auto = -1 */
bool opts_parse_fish_count(const char *val, int *out, char *errbuf, size_t errbuf_len);
/* reset creature flags. fish= only on allow_fish */
bool opts_parse_aquatic_life(const char *definition, struct aquatic_life *out, bool allow_fish, bool *fish_set, char *errbuf, size_t errbuf_len);
/* "" or 1.0 = 1, 1.1 = 2 */
bool opts_parse_classic(const char *val, int *out_ver, char *errbuf, size_t errbuf_len);
bool opts_parse_message_position(const char *val, enum message_position *out, char *errbuf, size_t errbuf_len);
bool opts_parse_uturn_chance(const char *val, int *out, char *errbuf, size_t errbuf_len);
bool opts_parse_fps(const char *val, int *out, char *errbuf, size_t errbuf_len);
bool opts_parse_pace(const char *s, double *out, char *errbuf, size_t errbuf_len);
/* out_cap incl NUL, printable ASCII (0x20-0x7E) */
bool opts_parse_teletext_caption(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len);
bool opts_parse_castle_name(const char *val, char *out, size_t out_cap, char *errbuf, size_t errbuf_len);

/* splits buf in place, drops trailing empty rows. caller frees rows */
int opts_split_lines(char *buf, char ***out_rows);

#endif
