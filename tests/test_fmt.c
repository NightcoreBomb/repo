/**
 * Тести Platform/fmt.c: власний форматер plat_vfmt (замінив vsnprintf, П-13)
 * дає той самий результат, що й snprintf, для усіх форматів, які вживає прошивка.
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "plat.h"

void setUp(void) { }
void tearDown(void) { }

#define CHECK(...) do { \
    char a[96], b[96]; \
    size_t n = plat_fmt(a, sizeof a, __VA_ARGS__); \
    int m = snprintf(b, sizeof b, __VA_ARGS__); \
    TEST_ASSERT_EQUAL_STRING(b, a); \
    TEST_ASSERT_EQUAL_size_t((size_t)m, n); \
  } while (0)

static void test_matches_snprintf(void)
{
  CHECK("plain");
  CHECK("%d %d %d", 0, -1, 2147483647);
  CHECK("%ld", (long)-2147483647 - 1);
  CHECK("%u %lu", 4294967295u, (unsigned long)123);
  CHECK("%x %X %02X %08lX", 255u, 255u, 5u, (unsigned long)0xC0FF8000u);
  CHECK("[%5d] [%-5d] [%05d] [%-4s] [%6s] [%c] %%", 42, 42, -42, "ab", "xyz", 'Q');
  CHECK("%03u|%3u|%-3u|", 7u, 7u, 7u);
  CHECK("EVT t_us=%lu name=%s", (unsigned long)123456789, "btn");
}

/* без атрибута format — щоб перевірити й «неправильні» рядки формату */
static size_t (*const raw_fmt)(char *, size_t, const char *, ...) = plat_fmt;

static void test_truncation_and_null(void)
{
  char a[8];
  size_t n = plat_fmt(a, sizeof a, "%s", "0123456789");
  TEST_ASSERT_EQUAL_size_t(10, n);                  /* як snprintf: повна довжина */
  TEST_ASSERT_EQUAL_STRING("0123456", a);
  raw_fmt(a, sizeof a, "%s", (const char *)0);
  TEST_ASSERT_EQUAL_STRING("(null)", a);
  raw_fmt(a, sizeof a, "%q%");
  TEST_ASSERT_EQUAL_STRING("%q", a);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_matches_snprintf);
  RUN_TEST(test_truncation_and_null);
  return UNITY_END();
}
