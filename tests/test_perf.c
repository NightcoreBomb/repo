/**
 * Тести App/perf.c: статистика задач, джитер (у т. ч. через переповнення 32-бітного
 * лічильника тактів), пропущені запуски, p99, завантаження CPU.
 */
#include "unity.h"
#include "perf.h"

void setUp(void) { }
void tearDown(void) { }

static void test_stat_min_avg_max(void)
{
  perf_stat_t s = { "x", 0, 0, 0, 0 };
  perf_reset(&s);
  TEST_ASSERT_EQUAL_UINT32(0, perf_avg_us(&s));
  perf_add_us(&s, 10); perf_add_us(&s, 30); perf_add_us(&s, 20);
  TEST_ASSERT_EQUAL_UINT32(3, s.n);
  TEST_ASSERT_EQUAL_UINT32(10, s.min_us);
  TEST_ASSERT_EQUAL_UINT32(30, s.max_us);
  TEST_ASSERT_EQUAL_UINT32(20, perf_avg_us(&s));
}

/* Лічильник тактів 48 МГц переповнюється кожні 89.5 с: різниця беззнакових коректна */
static void test_jitter_across_counter_wrap(void)
{
  perf_jit_t j;
  j.period_us = 5000;
  perf_jit_reset(&j);
  uint32_t t = 0xFFFFFFFFu - 48u * 7000u;
  for (int i = 0; i < 5; i++) { perf_jit_tick(&j, t, 48); t += 48u * 5000u; }
  TEST_ASSERT_EQUAL_UINT32(4, j.n);
  TEST_ASSERT_EQUAL_INT32(0, j.jmin_us);
  TEST_ASSERT_EQUAL_INT32(0, j.jmax_us);
  TEST_ASSERT_EQUAL_UINT32(0, j.missed);
}

static void test_jitter_missed_and_histogram(void)
{
  perf_jit_t j;
  j.period_us = 5000;
  perf_jit_reset(&j);
  uint32_t t = 0;
  perf_jit_tick(&j, t, 1);
  for (int i = 0; i < 98; i++) { t += 5000 + (i % 3) * 30; perf_jit_tick(&j, t, 1); }  /* 0/30/60 мкс */
  t += 60000; perf_jit_tick(&j, t, 1);              /* довга операція: 55 мс запізнення */
  t += 2000;  perf_jit_tick(&j, t, 1);              /* наздоганяє: −3 мс */
  TEST_ASSERT_EQUAL_UINT32(100, j.n);
  TEST_ASSERT_EQUAL_UINT32(1, j.missed);
  TEST_ASSERT_EQUAL_INT32(55000, j.jmax_us);
  TEST_ASSERT_EQUAL_INT32(-3000, j.jmin_us);
  TEST_ASSERT_EQUAL_UINT16(66, j.hist[0]);          /* < 50 мкс */
  TEST_ASSERT_EQUAL_UINT16(32, j.hist[1]);          /* 50..100 */
  TEST_ASSERT_EQUAL_UINT32(5000, perf_jit_p99_us(&j));   /* 99-й відлік — кошик 2..5 мс */
  perf_jit_t e;
  e.period_us = 1;
  perf_jit_reset(&e);
  TEST_ASSERT_EQUAL_UINT32(0, perf_jit_p99_us(&e));
}

static void test_load_window(void)
{
  perf_load_t l;
  perf_load_reset(&l, 100, 1000);
  perf_load_busy(&l, 250);
  TEST_ASSERT_EQUAL_INT(0, perf_load_poll(&l, 900));
  TEST_ASSERT_EQUAL_INT(1, perf_load_poll(&l, 1100));
  TEST_ASSERT_EQUAL_UINT8(25, l.last_pct);
  perf_load_busy(&l, 5000);                          /* більше за вікно → 100 % */
  TEST_ASSERT_EQUAL_INT(1, perf_load_poll(&l, 2100));
  TEST_ASSERT_EQUAL_UINT8(100, l.last_pct);
  TEST_ASSERT_EQUAL_UINT8(100, l.max_pct);
  TEST_ASSERT_EQUAL_UINT8(25, l.min_pct);
  TEST_ASSERT_EQUAL_UINT32(2, l.windows);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_stat_min_avg_max);
  RUN_TEST(test_jitter_across_counter_wrap);
  RUN_TEST(test_jitter_missed_and_histogram);
  RUN_TEST(test_load_window);
  return UNITY_END();
}
