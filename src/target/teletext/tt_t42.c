#define _DEFAULT_SOURCE

#include "teletext.h"
#include "../../xalloc.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#define TT_MAG 1
#define TT_PAGE 0x00
#define TT_PAGE_FILL 0xFF
#define PSI_PER_SEC 4
#define NATIONAL_OPTION_GERMAN 8

static const uint8_t ham84[16] = {0x15, 0x02, 0x49, 0x5E, 0x64, 0x73, 0x38, 0x2F, 0xD0, 0xC7, 0x8C, 0x9B, 0xA1, 0xB6, 0xFD, 0xEA};

static uint8_t odd_parity(uint8_t v) {
  v &= 0x7F;
  uint8_t p = v;
  p ^= (uint8_t)(p >> 4);
  p ^= (uint8_t)(p >> 2);
  p ^= (uint8_t)(p >> 1);
  return (p & 1) ? v : (uint8_t)(v | 0x80);
}

static void put_address(uint8_t out[TT_PACKET_LEN], int mag, int row) {
  out[0] = ham84[(mag & 7) | ((row & 1) << 3)];
  out[1] = ham84[(row >> 1) & 15];
}

void tt_t42_header(uint8_t out[TT_PACKET_LEN], int mag, int page, bool erase, const uint8_t text[TT_HDR_LEN]) {
  put_address(out, mag, 0);
  out[2] = ham84[page & 15];
  out[3] = ham84[(page >> 4) & 15];
  out[4] = ham84[0];
  out[5] = ham84[erase ? 8 : 0];
  out[6] = ham84[0];
  out[7] = ham84[0];
  out[8] = ham84[0];
  out[9] = ham84[NATIONAL_OPTION_GERMAN];
  for (int i = 0; i < TT_HDR_LEN; i++) out[10 + i] = odd_parity(text[i]);
}

void tt_t42_row(uint8_t out[TT_PACKET_LEN], int mag, int row, const uint8_t data[TT_COLS]) {
  put_address(out, mag, row);
  for (int i = 0; i < TT_COLS; i++) out[2 + i] = odd_parity(data[i]);
}

static unsigned bit(uint32_t d, int n) {
  return (d >> (n - 1)) & 1;
}

static void ham2418(uint8_t out[3], uint32_t d) {
  unsigned all = 0;
  for (int n = 1; n <= 18; n++) all ^= bit(d, n);
  unsigned p1 = 1 ^ bit(d, 1) ^ bit(d, 2) ^ bit(d, 4) ^ bit(d, 5) ^ bit(d, 7) ^ bit(d, 9) ^ bit(d, 11) ^ bit(d, 12) ^ bit(d, 14) ^ bit(d, 16) ^ bit(d, 18);
  unsigned p2 = 1 ^ bit(d, 1) ^ bit(d, 3) ^ bit(d, 4) ^ bit(d, 6) ^ bit(d, 7) ^ bit(d, 10) ^ bit(d, 11) ^ bit(d, 13) ^ bit(d, 14) ^ bit(d, 17) ^ bit(d, 18);
  unsigned p3 = 1 ^ bit(d, 2) ^ bit(d, 3) ^ bit(d, 4) ^ bit(d, 8) ^ bit(d, 9) ^ bit(d, 10) ^ bit(d, 11) ^ bit(d, 15) ^ bit(d, 16) ^ bit(d, 17) ^ bit(d, 18);
  unsigned p4 = 1 ^ bit(d, 5) ^ bit(d, 6) ^ bit(d, 7) ^ bit(d, 8) ^ bit(d, 9) ^ bit(d, 10) ^ bit(d, 11);
  unsigned p5 = 1;
  for (int n = 12; n <= 18; n++) p5 ^= bit(d, n);
  unsigned p6 = 1 ^ p1 ^ p2 ^ p3 ^ p4 ^ p5 ^ all;
  out[0] = (uint8_t)(p1 | p2 << 1 | bit(d, 1) << 2 | p3 << 3 | bit(d, 2) << 4 | bit(d, 3) << 5 | bit(d, 4) << 6 | p4 << 7);
  out[1] = (uint8_t)(((d >> 4) & 0x7F) | p5 << 7);
  out[2] = (uint8_t)(((d >> 11) & 0x7F) | p6 << 7);
}

void tt_t42_x26(uint8_t out[TT_PACKET_LEN], int mag, int dc, const struct tt_triplet t[TT_X26_TRIPLETS]) {
  put_address(out, mag, 26);
  out[2] = ham84[dc & 15];
  for (int i = 0; i < TT_X26_TRIPLETS; i++) {
    uint32_t d = (uint32_t)(t[i].addr & 63) | (uint32_t)(t[i].mode & 31) << 6 | (uint32_t)(t[i].data & 127) << 11;
    ham2418(out + 3 + 3 * i, d);
  }
}

bool tt_stdout_is_tty(void) {
#ifdef _WIN32
  return _isatty(_fileno(stdout)) != 0;
#else
  return isatty(STDOUT_FILENO) != 0;
#endif
}

void tt_sleep_ms(int ms) {
  if (ms <= 0) return;
#ifdef _WIN32
  Sleep((DWORD)ms);
#else
  struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
  nanosleep(&ts, NULL);
#endif
}

struct tt_stream {
  enum tt_mode mode;
  enum tt_glyphs glyphs;
  int x26_extent;
  struct tt_net *net;
  struct tt_ts ts;
  struct tt_page cur;
  struct tt_page prev;
  uint64_t frame;
  int fps;
  char caption[TT_HDR_LEN + 1];
  uint8_t pk[TT_MAX_PACKETS][TT_PACKET_LEN];
  uint8_t buf[TT_TS_PSI_LEN + TT_TS_LEN + TT_TS_PES_MAX];
};

struct tt_stream *tt_stream_open(enum tt_mode mode, enum tt_glyphs glyphs, struct tt_net *net, int fps, const char *caption) {
#ifdef _WIN32
  if (net == NULL && _setmode(_fileno(stdout), _O_BINARY) == -1) return NULL;
#endif
  struct tt_stream *s = xcalloc(1, sizeof(*s));
  s->mode = mode;
  s->glyphs = glyphs;
  s->net = net;
  s->fps = fps;
  size_t caption_len = strlen(caption);
  if (caption_len > TT_HDR_LEN) caption_len = TT_HDR_LEN;
  memcpy(s->caption, caption, caption_len);
  s->caption[caption_len] = '\0';
  tt_ts_init(&s->ts);
  return s;
}

static int emit(struct tt_stream *s, const uint8_t *buf, size_t len) {
  if (s->net == NULL) {
    if (fwrite(buf, 1, len, stdout) != len) return -1;
    return 0;
  }
  for (size_t off = 0; off < len;) {
    size_t n = len - off < 7 * TT_TS_LEN ? len - off : 7 * TT_TS_LEN;
    if (tt_net_send(s->net, buf + off, n) != 0) return -1;
    off += n;
  }
  return 0;
}

static void x26_packets(struct tt_stream *s, int *n) {
  int need = s->cur.ov_count > 0 ? (s->cur.ov_count + TT_X26_TRIPLETS) / TT_X26_TRIPLETS : 0;
  int send = need > s->x26_extent ? need : s->x26_extent;
  const struct tt_triplet end = {63, 31, 7};
  for (int dc = 0; dc < send; dc++) {
    struct tt_triplet t[TT_X26_TRIPLETS];
    for (int j = 0; j < TT_X26_TRIPLETS; j++) {
      int idx = dc * TT_X26_TRIPLETS + j;
      t[j] = idx < s->cur.ov_count ? s->cur.ov[idx] : end;
    }
    tt_t42_x26(s->pk[(*n)++], TT_MAG, dc, t);
  }
  s->x26_extent = need;
}

static int packetize(struct tt_stream *s) {
  int n = 0;
  uint8_t fill[TT_HDR_LEN];
  memset(fill, ' ', sizeof fill);
  memcpy(fill, s->caption, strlen(s->caption));
  tt_t42_header(s->pk[n++], TT_MAG, TT_PAGE, s->frame == 0, s->cur.row[0] + TT_HDR_COL);
  x26_packets(s, &n);
  int budget = TT_UNITS_PER_PES - 1 - n;
  int changed = 0;
  for (int r = 1; r < TT_ROWS; r++) changed += memcmp(s->cur.row[r], s->prev.row[r], TT_COLS) != 0;
  int spare = budget > changed ? budget - changed : 0;
  for (int r = 1; r < TT_ROWS; r++) {
    bool diff = memcmp(s->cur.row[r], s->prev.row[r], TT_COLS) != 0;
    bool refresh = !diff && spare > 0 && (r + s->frame) % 4 == 0;
    if (budget <= 0 || (!diff && !refresh)) continue;
    tt_t42_row(s->pk[n++], TT_MAG, r, s->cur.row[r]);
    memcpy(s->prev.row[r], s->cur.row[r], TT_COLS);
    budget--;
    if (refresh) spare--;
  }
  /* page ends at next header with other page number: filler page FF */
  tt_t42_header(s->pk[n++], TT_MAG, TT_PAGE_FILL, false, fill);
  return n;
}

int tt_stream_present(struct tt_stream *s, const struct canvas *c) {
  tt_render(c, s->glyphs, &s->cur, s->caption);
  int n = packetize(s);
  int rc;
  if (s->mode == TT_T42) {
    rc = emit(s, &s->pk[0][0], (size_t)n * TT_PACKET_LEN);
  } else {
    uint64_t pcr = s->frame * 27000000ULL / (uint64_t)s->fps;
    uint64_t pts = pcr / 300 + 27000;
    size_t len = 0;
    uint64_t psi_every = (uint64_t)(s->fps / PSI_PER_SEC > 0 ? s->fps / PSI_PER_SEC : 1);
    if (s->frame % psi_every == 0) {
      tt_ts_psi(&s->ts, s->buf);
      len += TT_TS_PSI_LEN;
    }
    tt_ts_pcr(&s->ts, s->buf + len, pcr);
    len += TT_TS_LEN;
    for (int off = 0; off < n; off += TT_UNITS_PER_PES) {
      int cnt = n - off < TT_UNITS_PER_PES ? n - off : TT_UNITS_PER_PES;
      len += tt_ts_pes(&s->ts, s->buf + len, (const uint8_t(*)[TT_PACKET_LEN])s->pk + off, cnt, pts + (uint64_t)(off / TT_UNITS_PER_PES) * 90);
    }
    rc = emit(s, s->buf, len);
  }
  s->frame++;
  if (rc == 0 && s->net == NULL && fflush(stdout) != 0) rc = -1;
  return rc;
}

void tt_stream_close(struct tt_stream *s) {
  free(s);
}
