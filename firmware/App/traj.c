#include <string.h>
#include "traj.h"

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static void put32(uint8_t *p, uint32_t v) { put16(p, (uint16_t)v); put16(p + 2, (uint16_t)(v >> 16)); }
static uint32_t get32(const uint8_t *p) { return get16(p) | ((uint32_t)get16(p + 2) << 16); }

uint16_t crc16_ccitt(const uint8_t *p, uint32_t n, uint16_t crc)
{
  while (n--) {
    crc ^= (uint16_t)(*p++ << 8);
    for (int k = 0; k < 8; k++) crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
  }
  return crc;
}

/* CRC блоку: усе, крім поля crc (байти 14..15) */
static uint16_t block_crc(const uint8_t *b)
{
  uint16_t c = crc16_ccitt(b, 14, 0xFFFFu);
  return crc16_ccitt(b + 16, TRAJ_BLOCK - 16u, c);
}

void traj_enc_begin(traj_enc_t *e, uint8_t *buf, uint16_t seq, uint32_t t0_ms)
{
  memset(buf, 0, TRAJ_BLOCK);
  e->buf = buf;
  e->seq = seq;
  e->n = 0;
  put16(buf, TRAJ_MAGIC_DATA);
  put16(buf + 2, seq);
  put32(buf + 10, t0_ms);
}

static int8_t sat8(int32_t v) { return (int8_t)(v > 127 ? 127 : (v < -127 ? -127 : v)); }

bool traj_enc_add(traj_enc_t *e, int16_t pan, int16_t tilt)
{
  if (e->n == 0) {
    put16(e->buf + 6, (uint16_t)pan);
    put16(e->buf + 8, (uint16_t)tilt);
    e->last_pan = pan;
    e->last_tilt = tilt;
  } else {
    int8_t dp = sat8(pan - e->last_pan), dt = sat8(tilt - e->last_tilt);
    uint8_t *q = e->buf + TRAJ_HDR + 2u * (e->n - 1u);
    q[0] = (uint8_t)dp;
    q[1] = (uint8_t)dt;
    e->last_pan  = (int16_t)(e->last_pan + dp);    /* як побачить декодер */
    e->last_tilt = (int16_t)(e->last_tilt + dt);
  }
  e->n++;
  return e->n >= TRAJ_PER_BLOCK;
}

uint16_t traj_enc_finish(traj_enc_t *e)
{
  put16(e->buf + 4, e->n);
  put16(e->buf + 14, block_crc(e->buf));
  return e->n;
}

int traj_block_check(const uint8_t *b, uint16_t seq)
{
  if (get16(b) != TRAJ_MAGIC_DATA || get16(b + 2) != seq) return -1;
  uint16_t n = get16(b + 4);
  if (n == 0 || n > TRAJ_PER_BLOCK) return -1;
  return (get16(b + 14) == block_crc(b)) ? (int)n : -1;
}

void traj_block_get(const uint8_t *b, uint16_t i, int16_t *pan, int16_t *tilt)
{
  for (uint16_t k = 0; k <= i; k++) traj_block_step(b, k, pan, tilt);
}

void traj_block_step(const uint8_t *b, uint16_t i, int16_t *pan, int16_t *tilt)
{
  if (i == 0) { *pan = (int16_t)get16(b + 6); *tilt = (int16_t)get16(b + 8); return; }
  const uint8_t *q = b + TRAJ_HDR + 2u * (i - 1u);
  *pan  = (int16_t)(*pan + (int8_t)q[0]);
  *tilt = (int16_t)(*tilt + (int8_t)q[1]);
}

void traj_dir_write(uint8_t *b, const traj_dir_t *d)
{
  memset(b, 0, TRAJ_BLOCK);
  put16(b, TRAJ_MAGIC_DIR);
  put16(b + 2, d->blocks);
  put32(b + 4, d->records);
  put32(b + 8, d->duration_ms);
  put16(b + 12, d->period_ms);
  put16(b + 14, crc16_ccitt(b, 14, 0xFFFFu));
}

bool traj_dir_read(const uint8_t *b, traj_dir_t *d)
{
  if (get16(b) != TRAJ_MAGIC_DIR || get16(b + 14) != crc16_ccitt(b, 14, 0xFFFFu)) return false;
  d->blocks = get16(b + 2);
  d->records = get32(b + 4);
  d->duration_ms = get32(b + 8);
  d->period_ms = get16(b + 12);
  return d->blocks <= TRAJ_MAX_BLOCKS;
}
