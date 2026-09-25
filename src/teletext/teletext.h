#ifndef UNDERTHEC_TELETEXT_H
#define UNDERTHEC_TELETEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../canvas.h"

#define TT_COLS 40
#define TT_ROWS 25
#define TT_PACKET_LEN 42
#define TT_X26_PACKETS 16
#define TT_X26_TRIPLETS 13
#define TT_MAX_TRIPLETS (TT_X26_PACKETS * TT_X26_TRIPLETS - 1)
/* header + rows 1-24 + terminating filler header */
#define TT_MAX_PACKETS (TT_ROWS + 1 + TT_X26_PACKETS)
/* row 0: decoder takes cols 0-7, stream owns TT_HDR_LEN */
#define TT_HDR_COL 8
#define TT_HDR_LEN (TT_COLS - TT_HDR_COL)
#define TT_TITLE "UNDERTHEC"
#define TT_CANVAS_H TT_ROWS

enum tt_mode {
  TT_T42,
  TT_TS
};

enum tt_glyphs {
  TT_MOSAIC,
  TT_TEXT
};

struct tt_triplet {
  uint8_t addr;
  uint8_t mode;
  uint8_t data;
};

struct tt_page {
  uint8_t row[TT_ROWS][TT_COLS];
  struct tt_triplet ov[TT_MAX_TRIPLETS];
  int ov_count;
};

struct tt_net;

int tt_canvas_w(enum tt_glyphs g);

void tt_render(const struct canvas *c, enum tt_glyphs g, struct tt_page *p, const char *caption);

/* mag 1-8, page BCD 0x00-0x99. text: TT_HDR_LEN page bytes for cols 8-39 */
void tt_t42_header(uint8_t out[TT_PACKET_LEN], int mag, int page, bool erase, const uint8_t text[TT_HDR_LEN]);
void tt_t42_row(uint8_t out[TT_PACKET_LEN], int mag, int row, const uint8_t data[TT_COLS]);

void tt_t42_x26(uint8_t out[TT_PACKET_LEN], int mag, int dc, const struct tt_triplet t[TT_X26_TRIPLETS]);

/* ts mux state */
struct tt_ts {
  uint8_t cc_pat;
  uint8_t cc_pmt;
  uint8_t cc_tt;
};

#define TT_TS_LEN 188
#define TT_TS_PSI_LEN (2 * TT_TS_LEN)
/* TT_MAX_PACKETS padded to 4k-1 data units, 46 bytes each, +PES hdr */
#define TT_UNITS_PER_PES 32
#define TT_TS_PES_MAX (((TT_MAX_PACKETS + TT_UNITS_PER_PES - 1) / TT_UNITS_PER_PES) * ((TT_UNITS_PER_PES + 4) / 4) * TT_TS_LEN)

void tt_ts_init(struct tt_ts *m);

/* PATPMT, TT_TS_PSI_LEN */
void tt_ts_psi(struct tt_ts *m, uint8_t *out);

/* PCR packet, 27 MHz ticks */
void tt_ts_pcr(const struct tt_ts *m, uint8_t *out, uint64_t pcr);

/* 1 to TT_MAX_PACKETS, PES in TS, PTS at 90 kHz */
size_t tt_ts_pes(struct tt_ts *m, uint8_t *out, const uint8_t (*pk)[TT_PACKET_LEN], int n, uint64_t pts);

/* spec=GROUP:PORT or [GROUP]:PORT. iface=local addr, name (IPv6), or NULL */
struct tt_net *tt_net_open(const char *spec, int ttl, const char *iface, char *errbuf, size_t errbuf_len);
int tt_net_send(const struct tt_net *n, const uint8_t *buf, size_t len);
void tt_net_close(struct tt_net *n);

struct tt_stream;

bool tt_stdout_is_tty(void);
void tt_sleep_ms(int ms);

/* net NULL=stdout. caption: row 0 label, truncated to TT_HDR_LEN */
struct tt_stream *tt_stream_open(enum tt_mode mode, enum tt_glyphs glyphs, struct tt_net *net, int fps, const char *caption);

/* 0 ok, -1 output err */
int tt_stream_present(struct tt_stream *s, const struct canvas *c);
void tt_stream_close(struct tt_stream *s);

#endif
