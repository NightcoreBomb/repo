/** Тести перетворень: кут ↔ імпульс серво (FR-05), АЦП → мВ / dead-zone (FR-04). */
#include "unity.h"
#include "servo.h"
#include "pot.h"

void setUp(void) { }
void tearDown(void) { }

static void test_servo_endpoints_and_center(void)
{
  TEST_ASSERT_EQUAL_UINT16(500,  servo_cdeg_to_us(0));
  TEST_ASSERT_EQUAL_UINT16(1500, servo_cdeg_to_us(9000));
  TEST_ASSERT_EQUAL_UINT16(2500, servo_cdeg_to_us(18000));
  TEST_ASSERT_EQUAL_UINT16(1000, servo_cdeg_to_us(4500));
  TEST_ASSERT_EQUAL_UINT16(2000, servo_cdeg_to_us(13500));
}

/* FR-05: при pitch = ±120° імпульс лишається 500..2500 мкс */
static void test_servo_clamps_out_of_range(void)
{
  TEST_ASSERT_EQUAL_UINT16(500,  servo_cdeg_to_us(-12000));
  TEST_ASSERT_EQUAL_UINT16(2500, servo_cdeg_to_us(21000));
  TEST_ASSERT_EQUAL_UINT16(500,  servo_clamp_us(0));
  TEST_ASSERT_EQUAL_UINT16(2500, servo_clamp_us(65000));
  TEST_ASSERT_EQUAL_UINT16(1234, servo_clamp_us(1234));
}

static void test_servo_roundtrip(void)
{
  for (int32_t c = 0; c <= 18000; c += 9) {
    int32_t back = servo_us_to_cdeg(servo_cdeg_to_us(c));
    TEST_ASSERT_INT32_WITHIN(5, c, back);          /* крок 1 мкс = 0.09° */
  }
  TEST_ASSERT_EQUAL_INT32(0, servo_us_to_cdeg(100));
}

static void test_pot_conversion(void)
{
  TEST_ASSERT_EQUAL_UINT32(0,    pot_raw_to_mv(0));
  TEST_ASSERT_EQUAL_UINT32(3300, pot_raw_to_mv(4095));
  TEST_ASSERT_EQUAL_UINT32(1650, pot_raw_to_mv(2048));
  TEST_ASSERT_EQUAL_UINT32(2384, pot_raw_to_mv(2958));    /* значення з UART-логу ЛР1: 2958 → 2383 мВ (там усічення) */
  TEST_ASSERT_EQUAL_UINT32(3300, pot_raw_to_mv(9999));
  TEST_ASSERT_EQUAL_INT32(0,    pot_raw_to_deadzone_cdeg(0));
  TEST_ASSERT_EQUAL_INT32(1000, pot_raw_to_deadzone_cdeg(4095));
  TEST_ASSERT_EQUAL_INT32(500,  pot_raw_to_deadzone_cdeg(2048));
  TEST_ASSERT_EQUAL_INT32(1000, pot_raw_to_deadzone_cdeg(5000));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_servo_endpoints_and_center);
  RUN_TEST(test_servo_clamps_out_of_range);
  RUN_TEST(test_servo_roundtrip);
  RUN_TEST(test_pot_conversion);
  return UNITY_END();
}
