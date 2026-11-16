/**
 * perf.h — вимірювання швидкодії (методичка ЛР3 §5.1). Чиста логіка над «тактами»:
 * джерело тактів — plat_cycles() (на M0+ — SysTick, бо DWT CYCCNT немає).
 *
 * Формат виводу (розбирає tools/lr3_metrics.py):
 *   STAT name=<task> n=<..> min_us=<..> avg_us=<..> max_us=<..>
 *   JIT  name=<task> period_us=<..> n=<..> min_us=<..> max_us=<..> p99_us=<..> missed=<..>
 *   LAT  name=<event> n=<..> min_us=<..> avg_us=<..> max_us=<..>
 */
#ifndef APP_PERF_H
#define APP_PERF_H

#include <stdint.h>

typedef struct {
  const char *name;
  uint32_t n, min_us, max_us, sum_us;
} perf_stat_t;

void perf_reset(perf_stat_t *s);
void perf_add_us(perf_stat_t *s, uint32_t us);
uint32_t perf_avg_us(const perf_stat_t *s);

/* Джитер періодичної задачі: відхилення фактичного інтервалу між запусками від
   заданого періоду. Гістограма |джитера| — межі у PERF_JH_EDGES. */
#define PERF_JH_BINS 12
extern const uint32_t perf_jh_edges_us[PERF_JH_BINS];

typedef struct {
  const char *name;
  uint32_t period_us;
  uint32_t prev_cyc;
  int32_t  jmin_us, jmax_us;
  uint32_t n, missed;                  /* missed: інтервал > 2 періодів */
  uint16_t hist[PERF_JH_BINS];
  uint8_t  primed;
} perf_jit_t;

void perf_jit_reset(perf_jit_t *j);
/* now_cyc — момент запуску; cyc_per_us — тактів на мкс */
void perf_jit_tick(perf_jit_t *j, uint32_t now_cyc, uint32_t cyc_per_us);
/* Оцінка 99-го перцентиля |джитера| за гістограмою (верхня межа кошика) */
uint32_t perf_jit_p99_us(const perf_jit_t *j);

/* Завантаження CPU: частка часу, коли задачі виконували роботу, у вікні 1 с */
typedef struct {
  uint32_t win_start, busy, win_cyc;
  uint8_t  last_pct, max_pct, min_pct;
  uint32_t windows;
} perf_load_t;

void perf_load_reset(perf_load_t *l, uint32_t now_cyc, uint32_t win_cyc);
void perf_load_busy(perf_load_t *l, uint32_t cyc);
/* true — вікно закрилося (last_pct оновлено) */
int  perf_load_poll(perf_load_t *l, uint32_t now_cyc);

#endif
