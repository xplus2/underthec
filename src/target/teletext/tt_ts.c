#include "teletext.h"

#include <string.h>

#define PID_PAT 0x0000
#define PID_PMT 0x1000
#define PID_TT 0x0100
#define PID_PCR 0x0101
#define PROGRAM 1
#define TS_PAYLOAD (TT_TS_LEN - 4)
#define UNIT_LEN 46
#define PES_HDR_DATA 0x24
#define PTS_MASK 0x1FFFFFFFFULL

static uint32_t crc32_mpeg(const uint8_t *p, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint32_t)p[i] << 24;
    for (int b = 0; b < 8; b++) crc = (crc & 0x80000000u) ? (crc << 1) ^ 0x04C11DB7u : crc << 1;
  }
  return crc;
}

static uint8_t rev8(uint8_t v) {
  v = (uint8_t)(((v & 0xF0) >> 4) | ((v & 0x0F) << 4));
  v = (uint8_t)(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
  v = (uint8_t)(((v & 0xAA) >> 1) | ((v & 0x55) << 1));
  return v;
}

static void ts_header(uint8_t *out, int pid, bool pusi, int afc, uint8_t cc) {
  out[0] = 0x47;
  out[1] = (uint8_t)((pusi ? 0x40 : 0) | ((pid >> 8) & 0x1F));
  out[2] = (uint8_t)(pid & 0xFF);
  out[3] = (uint8_t)((afc << 4) | (cc & 15));
}

static void put_crc(uint8_t *sec, size_t len) {
  uint32_t crc = crc32_mpeg(sec, len);
  sec[len] = (uint8_t)(crc >> 24);
  sec[len + 1] = (uint8_t)(crc >> 16);
  sec[len + 2] = (uint8_t)(crc >> 8);
  sec[len + 3] = (uint8_t)crc;
}

static void psi_packet(uint8_t *out, int pid, uint8_t *cc, const uint8_t *sec, size_t sec_len) {
  ts_header(out, pid, true, 1, (*cc)++);
  out[4] = 0;
  memcpy(out + 5, sec, sec_len);
  memset(out + 5 + sec_len, 0xFF, TT_TS_LEN - 5 - sec_len);
}

void tt_ts_init(struct tt_ts *m) {
  memset(m, 0, sizeof(*m));
}

void tt_ts_psi(struct tt_ts *m, uint8_t *out) {
  uint8_t pat[16] = {0x00, 0xB0, 13, 0x00, PROGRAM, 0xC1, 0x00, 0x00, 0x00, PROGRAM, 0xE0 | (PID_PMT >> 8), PID_PMT & 0xFF};
  put_crc(pat, 12);
  psi_packet(out, PID_PAT, &m->cc_pat, pat, 16);

  uint8_t pmt[28] = {
    0x02, 0xB0, 25, 0x00, PROGRAM, 0xC1, 0x00, 0x00, 0xE0 | (PID_PCR >> 8), PID_PCR & 0xFF, 0xF0, 0x00,
    0x06, 0xE0 | (PID_TT >> 8), PID_TT & 0xFF, 0xF0, 7, 0x56, 5, 'u', 'n', 'd', (1 << 3) | 1, 0x00
  };
  put_crc(pmt, 24);
  psi_packet(out + TT_TS_LEN, PID_PMT, &m->cc_pmt, pmt, 28);
}

void tt_ts_pcr(const struct tt_ts *m, uint8_t *out, uint64_t pcr) {
  (void)m;
  uint64_t base = (pcr / 300) & PTS_MASK;
  unsigned ext = (unsigned)(pcr % 300);
  ts_header(out, PID_PCR, false, 2, 0);
  out[4] = TT_TS_LEN - 5;
  out[5] = 0x10;
  out[6] = (uint8_t)(base >> 25);
  out[7] = (uint8_t)(base >> 17);
  out[8] = (uint8_t)(base >> 9);
  out[9] = (uint8_t)(base >> 1);
  out[10] = (uint8_t)(((base & 1) << 7) | 0x7E | (ext >> 8));
  out[11] = (uint8_t)(ext & 0xFF);
  memset(out + 12, 0xFF, TT_TS_LEN - 12);
}

static void put_pts(uint8_t *out, uint64_t pts) {
  pts &= PTS_MASK;
  out[0] = (uint8_t)(0x21 | (((pts >> 30) & 7) << 1));
  out[1] = (uint8_t)(pts >> 22);
  out[2] = (uint8_t)((((pts >> 15) & 0x7F) << 1) | 1);
  out[3] = (uint8_t)(pts >> 7);
  out[4] = (uint8_t)(((pts & 0x7F) << 1) | 1);
}

size_t tt_ts_pes(struct tt_ts *m, uint8_t *out, const uint8_t (*pk)[TT_PACKET_LEN], int n, uint64_t pts) {
  size_t k = ((size_t)n + 1 + 3) / 4;
  size_t units = 4 * k - 1;
  size_t total = UNIT_LEN * (units + 1);
  uint8_t pes[TT_TS_PES_MAX];
  uint8_t *p = pes;
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x01;
  *p++ = 0xBD;
  *p++ = (uint8_t)((total - 6) >> 8);
  *p++ = (uint8_t)((total - 6) & 0xFF);
  *p++ = 0x84;
  *p++ = 0x80;
  *p++ = PES_HDR_DATA;
  put_pts(p, pts);
  memset(p + 5, 0xFF, PES_HDR_DATA - 5);
  p += PES_HDR_DATA;
  *p++ = 0x10;
  for (size_t u = 0; u < units; u++) {
    if ((int)u < n) {
      *p++ = 0x02;
      *p++ = 0x2C;
      *p++ = 0xE0;
      *p++ = rev8(0x27);
      for (int i = 0; i < TT_PACKET_LEN; i++) *p++ = rev8(pk[u][i]);
    } else {
      *p++ = 0xFF;
      *p++ = 0x2C;
      memset(p, 0xFF, 44);
      p += 44;
    }
  }
  for (size_t off = 0, i = 0; off < total; off += TS_PAYLOAD, i++) {
    uint8_t *dst = out + i * TT_TS_LEN;
    ts_header(dst, PID_TT, off == 0, 1, m->cc_tt++);
    memcpy(dst + 4, pes + off, TS_PAYLOAD);
  }
  return total / TS_PAYLOAD * TT_TS_LEN;
}
