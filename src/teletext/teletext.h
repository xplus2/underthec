#ifndef UNDERTHEC_TELETEXT_H
#define UNDERTHEC_TELETEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../canvas.h"

#define TT_COLS 40
#define TT_ROWS 24
#define TT_PACKET_LEN 42
/* header + rows 1-23 + terminating filler header */
#define TT_MAX_PACKETS (TT_ROWS + 1)
/* tank size: col 0 control cell, row 0 header */
#define TT_CANVAS_W ((TT_COLS - 1) * 2)
#define TT_CANVAS_H (TT_ROWS - 1)

enum tt_mode {
  TT_T42,
  TT_TS
};

struct tt_page {
  uint8_t row[TT_ROWS][TT_COLS];
};

struct tt_net;

/* rows 1-23 from canvas (TT_CANVAS_W x TT_CANVAS_H) */
void tt_render(const struct canvas *c, struct tt_page *p);

/* mag 1-8, page BCD 0x00-0x99. text: up to 32c */
void tt_t42_header(uint8_t out[TT_PACKET_LEN], int mag, int page, bool erase, const char *text);
void tt_t42_row(uint8_t out[TT_PACKET_LEN], int mag, int row, const uint8_t data[TT_COLS]);

/* ts mux state */
struct tt_ts {
  uint8_t cc_pat;
  uint8_t cc_pmt;
  uint8_t cc_tt;
};

#define TT_TS_LEN 188
#define TT_TS_PSI_LEN (2 * TT_TS_LEN)
/* TT_MAX_PACKETS padded to 4k-1 data units, 46 bytes each, +PES hdr */
#define TT_TS_PES_MAX (7 * TT_TS_LEN)

void tt_ts_init(struct tt_ts *m);

/* PATPMT, TT_TS_PSI_LEN */
void tt_ts_psi(struct tt_ts *m, uint8_t *out);

/* PCR packet, 27 MHz ticks */
void tt_ts_pcr(struct tt_ts *m, uint8_t *out, uint64_t pcr);

/* 1 to TT_MAX_PACKETS, PES in TS, PTS at 90 kHz */
size_t tt_ts_pes(struct tt_ts *m, uint8_t *out, const uint8_t (*pk)[TT_PACKET_LEN], int n, uint64_t pts);

/* spec=GROUP:PORT or [GROUP]:PORT. iface=local addr, name (IPv6), or NULL */
struct tt_net *tt_net_open(const char *spec, int ttl, const char *iface, char *errbuf, size_t errbuf_len);
int tt_net_send(struct tt_net *n, const uint8_t *buf, size_t len);
void tt_net_close(struct tt_net *n);

struct tt_stream;

bool tt_stdout_is_tty(void);
void tt_sleep_ms(int ms);

/* net NULL=stdout */
struct tt_stream *tt_stream_open(enum tt_mode mode, struct tt_net *net, int fps);

/* 0 ok, -1 output err */
int tt_stream_present(struct tt_stream *s, const struct canvas *c);
void tt_stream_close(struct tt_stream *s);

#endif
