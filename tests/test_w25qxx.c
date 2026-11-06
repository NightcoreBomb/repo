/**
 * Тести W25Q64 (Форма A). Головне — ПОСЛІДОВНІСТЬ команд на шині:
 * перед Page Program / Sector Erase має бути WREN (06h), команда+адреса+дані —
 * під одним CS#, після запису — опитування BUSY. Перевіряється журналом SPI
 * (mock-підхід) і моделлю чипа, що ігнорує запис без WEL (fake-підхід).
 */
#include <string.h>
#include "unity.h"
#include "w25qxx.h"
#include "spi_log.h"
#include "fake_w25q.h"

static w25q_t d;
void setUp(void) { fake_w25q_reset(); spi_log_reset(&fake_w25q_dev); w25q_setup(&d, &spi_log_bus); }
void tearDown(void) { }

static uint8_t first(int k) { uint8_t b[1] = { 0 }; spi_frame(k, b, NULL, 1); return b[0]; }

static void test_jedec_id(void)
{
  TEST_ASSERT_EQUAL_INT(ST_OK, w25q_identify(&d));
  TEST_ASSERT_EQUAL_HEX32(0xEF4017, d.jedec);
  TEST_ASSERT_EQUAL_INT(1, spi_frame_count());
  uint8_t mo[4]; TEST_ASSERT_EQUAL_INT(4, spi_frame(0, mo, NULL, 4));
  TEST_ASSERT_EQUAL_HEX8(0x9F, mo[0]);
}

static void test_wrong_jedec(void)
{
  fake_w25q.jedec = 0xC22017;               /* Macronix */
  TEST_ASSERT_EQUAL_INT(ST_EID, w25q_identify(&d));
}

static void test_page_program_is_preceded_by_write_enable(void)
{
  const uint8_t data[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
  TEST_ASSERT_EQUAL_INT(ST_OK, w25q_page_program(&d, 0x000100, data, 4));
  /* кадри: [05 busy?] [06 WREN] [05 перевірка WEL] [02 a a a d d d d] [05..] */
  int pp = -1;
  for (int k = 0; k < spi_frame_count(); k++) if (first(k) == 0x02) { pp = k; break; }
  TEST_ASSERT_TRUE(pp >= 2);
  TEST_ASSERT_EQUAL_HEX8(0x05, first(pp - 1));   /* перевірили WEL ... */
  TEST_ASSERT_EQUAL_HEX8(0x06, first(pp - 2));   /* ... після WREN */
  uint8_t f[16]; int n = spi_frame(pp, f, NULL, sizeof f);
  TEST_ASSERT_EQUAL_INT(8, n);                    /* команда+адреса+дані під одним CS# */
  const uint8_t exp[8] = { 0x02, 0x00, 0x01, 0x00, 0xDE, 0xAD, 0xBE, 0xEF };
  TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, f, 8);
  TEST_ASSERT_EQUAL_HEX8(0x05, first(spi_frame_count() - 1));  /* дочекались BUSY=0 */
  TEST_ASSERT_EQUAL_INT(0, fake_w25q.ignored_programs);
  uint8_t r[4];
  TEST_ASSERT_EQUAL_INT(ST_OK, w25q_read(&d, 0x100, r, 4));
  TEST_ASSERT_EQUAL_HEX8_ARRAY(data, r, 4);
}

static void test_program_aborts_if_wel_not_set(void)
{
  const uint8_t data[1] = { 0x00 };
  fake_w25q.wel_stuck_low = true;           /* WP# активний / чип захищений */
  TEST_ASSERT_EQUAL_INT(ST_EVERIFY, w25q_page_program(&d, 0, data, 1));
  for (int k = 0; k < spi_frame_count(); k++) TEST_ASSERT_NOT_EQUAL(0x02, first(k));
}

static void test_sector_erase_sequence_and_effect(void)
{
  const uint8_t z[2] = { 0, 0 };
  w25q_page_program(&d, 0x1010, z, 2);
  int from = spi_frame_count();
  TEST_ASSERT_EQUAL_INT(ST_OK, w25q_sector_erase(&d, 0x1234));
  int se = -1;
  for (int k = from; k < spi_frame_count(); k++) if (first(k) == 0x20) { se = k; break; }
  TEST_ASSERT_TRUE(se >= 0);
  TEST_ASSERT_EQUAL_HEX8(0x06, first(se - 2));
  uint8_t f[4]; spi_frame(se, f, NULL, 4);
  TEST_ASSERT_EQUAL_HEX8(0x10, f[2]);        /* адреса вирівняна до 4 КБ: 0x001000 */
  TEST_ASSERT_EQUAL_HEX8(0x00, f[3]);
  uint8_t r[2]; w25q_read(&d, 0x1010, r, 2);
  TEST_ASSERT_EQUAL_HEX8(0xFF, r[0]);
}

static void test_page_boundary_rejected(void)
{
  uint8_t b[8] = { 0 };
  TEST_ASSERT_EQUAL_INT(ST_EPARAM, w25q_page_program(&d, 0xFC, b, 8));   /* 0xFC+8 > 0x100 */
  TEST_ASSERT_EQUAL_INT(ST_EPARAM, w25q_page_program(&d, 0, b, 0));
  TEST_ASSERT_EQUAL_INT(0, spi_frame_count());                          /* на шину — нічого */
}

static void test_busy_timeout(void)
{
  const uint8_t b[1] = { 0 };
  fake_w25q.busy_after_op = 1000000;
  d.busy_poll_max = 50;
  TEST_ASSERT_EQUAL_INT(ST_ETIMEOUT, w25q_page_program(&d, 0, b, 1));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_jedec_id);
  RUN_TEST(test_wrong_jedec);
  RUN_TEST(test_page_program_is_preceded_by_write_enable);
  RUN_TEST(test_program_aborts_if_wel_not_set);
  RUN_TEST(test_sector_erase_sequence_and_effect);
  RUN_TEST(test_page_boundary_rejected);
  RUN_TEST(test_busy_timeout);
  return UNITY_END();
}
