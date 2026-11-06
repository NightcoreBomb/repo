#include <stdio.h>
#include <string.h>
#include "mock_i2c.h"

#define MAXE 160
typedef struct { mi_op_t op; uint8_t addr; uint8_t w[32]; size_t nw; uint8_t r[32]; size_t nr; int ret; } exp_t;
static exp_t e[MAXE];
static int n_exp, cur;
static char err[160];

void mock_i2c_reset(void) { n_exp = cur = 0; err[0] = 0; }

void mock_i2c_expect(mi_op_t op, uint8_t addr, const uint8_t *w, size_t nw,
                     const uint8_t *r, size_t nr, int ret)
{
  if (n_exp >= MAXE) { snprintf(err, sizeof err, "too many expectations"); return; }
  exp_t *x = &e[n_exp++];
  x->op = op; x->addr = addr; x->nw = nw; x->nr = nr; x->ret = ret;
  if (nw) memcpy(x->w, w, nw);
  if (nr) memcpy(x->r, r, nr);
}

static exp_t *next(mi_op_t op, uint8_t addr, const uint8_t *w, size_t nw)
{
  if (err[0]) return NULL;
  if (cur >= n_exp) { snprintf(err, sizeof err, "unexpected call #%d op=%d addr=0x%02X", cur, op, addr); return NULL; }
  exp_t *x = &e[cur];
  if (x->op != op || x->addr != addr) {
    snprintf(err, sizeof err, "call #%d: expected op=%d addr=0x%02X, got op=%d addr=0x%02X",
             cur, x->op, x->addr, op, addr);
    return NULL;
  }
  if (x->nw != nw || (nw && memcmp(x->w, w, nw))) {
    snprintf(err, sizeof err, "call #%d: write bytes differ (len %zu vs %zu, first 0x%02X vs 0x%02X)",
             cur, x->nw, nw, x->nw ? x->w[0] : 0, nw ? w[0] : 0);
    return NULL;
  }
  cur++;
  return x;
}

static int m_write(void *c, uint8_t a, const uint8_t *d, size_t n)
{ (void)c; exp_t *x = next(MI_WRITE, a, d, n); return x ? x->ret : ST_EIO; }

static int m_read(void *c, uint8_t a, uint8_t *d, size_t n)
{
  (void)c; exp_t *x = next(MI_READ, a, NULL, 0);
  if (!x) return ST_EIO;
  memcpy(d, x->r, n < x->nr ? n : x->nr);
  return x->ret;
}

static int m_wr(void *c, uint8_t a, const uint8_t *w, size_t nw, uint8_t *r, size_t nr)
{
  (void)c; exp_t *x = next(MI_WRITE_READ, a, w, nw);
  if (!x) return ST_EIO;
  if (x->ret == ST_OK) memcpy(r, x->r, nr < x->nr ? nr : x->nr);
  return x->ret;
}

static int m_probe(void *c, uint8_t a)
{ (void)c; exp_t *x = next(MI_PROBE, a, NULL, 0); return x ? x->ret : ST_EIO; }

const char *mock_i2c_verify(void)
{
  if (err[0]) return err;
  if (cur != n_exp) { snprintf(err, sizeof err, "only %d of %d expected calls made", cur, n_exp); return err; }
  return NULL;
}

const bus_i2c_t mock_i2c_bus = { NULL, m_write, m_read, m_wr, m_probe };
