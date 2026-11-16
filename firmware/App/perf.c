#include "perf.h"

void perf_reset(perf_stat_t *s)
{
  s->n = s->max_us = s->sum_us = 0;
  s->min_us = 0xFFFFFFFFu;
}

void perf_add_us(perf_stat_t *s, uint32_t us)
{
  if (us < s->min_us) s->min_us = us;
  if (us > s->max_us) s->max_us = us;
  s->sum_us += us;
  s->n++;
}

uint32_t perf_avg_us(const perf_stat_t *s) { return s->n ? s->sum_us / s->n : 0; }

/* межі кошиків |джитера|, мкс; останній — «≥ 100 мс» */
const uint32_t perf_jh_edges_us[PERF_JH_BINS] = {
  50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 0xFFFFFFFFu
};

void perf_jit_reset(perf_jit_t *j)
{
  j->primed = 0;
  j->n = j->missed = 0;
  j->jmin_us = 0x7FFFFFFF;
  j->jmax_us = -0x7FFFFFFF;
  for (int i = 0; i < PERF_JH_BINS; i++) j->hist[i] = 0;
}

void perf_jit_tick(perf_jit_t *j, uint32_t now, uint32_t cpu)
{
  if (j->primed) {
    uint32_t dt = (now - j->prev_cyc) / cpu;
    int32_t jit = (int32_t)dt - (int32_t)j->period_us;
    if (jit < j->jmin_us) j->jmin_us = jit;
    if (jit > j->jmax_us) j->jmax_us = jit;
    if (dt > 2u * j->period_us) j->missed++;
    uint32_t a = (uint32_t)(jit < 0 ? -jit : jit);
    int b = 0;
    while (b < PERF_JH_BINS - 1 && a >= perf_jh_edges_us[b]) b++;
    if (j->hist[b] < 0xFFFFu) j->hist[b]++;
    j->n++;
  }
  j->prev_cyc = now;
  j->primed = 1;
}

uint32_t perf_jit_p99_us(const perf_jit_t *j)
{
  uint32_t total = 0, acc = 0;
  for (int i = 0; i < PERF_JH_BINS; i++) total += j->hist[i];
  if (!total) return 0;
  uint32_t need = total - total / 100u;          /* ≥ 99 % відліків */
  for (int i = 0; i < PERF_JH_BINS; i++) {
    acc += j->hist[i];
    if (acc >= need) return perf_jh_edges_us[i];
  }
  return perf_jh_edges_us[PERF_JH_BINS - 1];
}

void perf_load_reset(perf_load_t *l, uint32_t now, uint32_t win)
{
  l->win_start = now;
  l->busy = 0;
  l->win_cyc = win;
  l->last_pct = l->max_pct = 0;
  l->min_pct = 100;
  l->windows = 0;
}

void perf_load_busy(perf_load_t *l, uint32_t cyc) { l->busy += cyc; }

int perf_load_poll(perf_load_t *l, uint32_t now)
{
  uint32_t w = now - l->win_start;
  if (w < l->win_cyc) return 0;
  uint32_t pct = l->busy / (w / 100u);          /* без 64-бітного ділення (__aeabi_uldivmod) */
  if (pct > 100u) pct = 100u;
  l->last_pct = (uint8_t)pct;
  if (pct > l->max_pct) l->max_pct = (uint8_t)pct;
  if (pct < l->min_pct) l->min_pct = (uint8_t)pct;
  l->windows++;
  l->busy = 0;
  l->win_start = now;
  return 1;
}
