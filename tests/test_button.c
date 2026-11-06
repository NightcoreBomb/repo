/**
 * Тести debounce кнопки (Devices/button.c). Сирий рівень подається «по мілісекундах»,
 * як у SysTick. Головний сценарій — П-12: брязкіт при ВІДПУСКАННІ не має давати
 * другого натискання (стара реалізація давала, якщо кнопку тримали > 50 мс).
 */
#include "unity.h"
#include "button.h"

static button_t b;
static uint32_t t;
static int presses_seen;

void setUp(void) { t = 0; presses_seen = 0; button_init(&b, 1, 0, 50); }
void tearDown(void) { }

/* подати рівень протягом ms мілісекунд */
static void hold(int level, uint32_t ms)
{
  for (uint32_t i = 0; i < ms; i++, t++)
    if (button_update(&b, level, t) == BTN_EV_PRESS) presses_seen++;
}

/* брязкіт: n перемикань по 1 мс, закінчується рівнем end */
static void bounce(int end, int n)
{
  for (int i = 0; i < n; i++) hold(((i & 1) == 0) ? end : !end, 1);
  hold(end, 1);
}

static void test_clean_press_and_release_count_once(void)
{
  hold(1, 100); hold(0, 200); hold(1, 200);
  TEST_ASSERT_EQUAL_UINT32(1, b.presses);
  TEST_ASSERT_EQUAL_UINT32(1, b.releases);
  TEST_ASSERT_EQUAL_INT(1, presses_seen);
}

/* П-12: тримаємо 300 мс, брязкіт і на натисканні, і на відпусканні → рівно 1 */
static void test_bounce_on_release_is_not_a_second_press(void)
{
  hold(1, 100);
  bounce(0, 9);   hold(0, 300);
  bounce(1, 9);   hold(1, 300);
  TEST_ASSERT_EQUAL_UINT32(1, b.presses);
  TEST_ASSERT_EQUAL_UINT32(1, b.releases);
}

/* PRD, приймальний тест FR-06: 20 натискань, у т. ч. з брязкотом → рівно 20 */
static void test_twenty_presses_with_bounce(void)
{
  for (int k = 0; k < 20; k++) {
    if (k % 2) { bounce(0, 7); hold(0, 120); bounce(1, 5); hold(1, 120); }
    else       { hold(0, 80); hold(1, 80); }
  }
  TEST_ASSERT_EQUAL_UINT32(20, b.presses);
  TEST_ASSERT_EQUAL_UINT32(20, b.releases);
}

/* Відтворення ДЕФЕКТУ П-12: стара логіка (Platform/plat_stm32.c до виправлення) —
   «EXTI по спаду; рахувати, якщо від попереднього зарахованого минуло > 50 мс» —
   на тих самих вхідних даних дає 2 натискання замість 1. */
static void test_old_falling_edge_debounce_counted_release_bounce(void)
{
  static const struct { int level; uint32_t ms; } in[] = {
    {1,100}, {0,1},{1,1},{0,1},{1,1},{0,300},          /* натискання з брязкотом, тримаємо 300 мс */
    {1,1},{0,1},{1,1},{0,1},{1,300},                    /* відпускання з брязкотом */
  };
  uint32_t tt = 0, last = 0, old_presses = 0; int prev = 1;
  for (unsigned i = 0; i < sizeof in / sizeof in[0]; i++)
    for (uint32_t k = 0; k < in[i].ms; k++, tt++) {
      if (prev == 1 && in[i].level == 0 && tt - last > 50u) { old_presses++; last = tt; }
      prev = in[i].level;
      if (button_update(&b, in[i].level, tt) == BTN_EV_PRESS) presses_seen++;
    }
  TEST_ASSERT_EQUAL_UINT32(2, old_presses);   /* старий алгоритм: натискання + «відтискання» */
  TEST_ASSERT_EQUAL_INT(1, presses_seen);     /* новий: рівно одне */
}

static void test_short_glitch_is_ignored(void)
{
  hold(1, 100); hold(0, 30); hold(1, 200);       /* < 50 мс — завада, не натискання */
  TEST_ASSERT_EQUAL_UINT32(0, b.presses);
}

static void test_press_reported_after_exactly_debounce_time(void)
{
  hold(1, 10);
  uint32_t t_press = t;
  while (b.presses == 0 && t < 1000) hold(0, 1);
  TEST_ASSERT_EQUAL_UINT32(50, t - t_press - 1);   /* затримка = вікно debounce */
}

static void test_tick_counter_wraparound(void)
{
  t = 0xFFFFFFF0u;
  button_init(&b, 1, t, 50);
  hold(1, 5); hold(0, 100); hold(1, 100);         /* перехід через 0 посередині */
  TEST_ASSERT_EQUAL_UINT32(1, b.presses);
}

static void test_held_at_boot_needs_release_first(void)
{
  button_init(&b, 0, 0, 50);                      /* затиснута під час старту */
  hold(0, 300);
  TEST_ASSERT_EQUAL_UINT32(0, b.presses);         /* не «натискання» */
  hold(1, 100); hold(0, 100);
  TEST_ASSERT_EQUAL_UINT32(1, b.presses);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_clean_press_and_release_count_once);
  RUN_TEST(test_bounce_on_release_is_not_a_second_press);
  RUN_TEST(test_twenty_presses_with_bounce);
  RUN_TEST(test_old_falling_edge_debounce_counted_release_bounce);
  RUN_TEST(test_short_glitch_is_ignored);
  RUN_TEST(test_press_reported_after_exactly_debounce_time);
  RUN_TEST(test_tick_counter_wraparound);
  RUN_TEST(test_held_at_boot_needs_release_first);
  return UNITY_END();
}
