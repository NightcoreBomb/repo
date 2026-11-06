/** Тести драйвера SSD1306: формат кадрів I²C (control byte), розбиття даних, помилки. */
#include <string.h>
#include "unity.h"
#include "ssd1306.h"
#include "mock_i2c.h"
#include "stub_i2c.h"

static ssd1306_t d;
void setUp(void) { mock_i2c_reset(); ssd1306_setup(&d, &mock_i2c_bus, 0x3C); }
void tearDown(void) { }

static void test_probe_is_address_only(void)
{
  mock_i2c_expect(MI_PROBE, 0x3C, NULL, 0, NULL, 0, ST_OK);
  TEST_ASSERT_EQUAL_INT(ST_OK, ssd1306_probe(&d));
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_init_is_single_command_stream_ending_with_display_on(void)
{
  const uint8_t exp[] = { 0x00, 0xAE, 0xD5,0x80, 0xA8,0x3F, 0xD3,0x00, 0x40, 0x8D,0x14,
                          0x20,0x00, 0xA1, 0xC8, 0xDA,0x12, 0x81,0xCF, 0xD9,0xF1, 0xDB,0x40,
                          0xA4, 0xA6, 0xAF };
  mock_i2c_expect(MI_WRITE, 0x3C, exp, sizeof exp, NULL, 0, ST_OK);
  TEST_ASSERT_EQUAL_INT(ST_OK, ssd1306_init(&d));
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_text_sets_window_then_sends_glyph_data(void)
{
  const uint8_t win[] = { 0x00, 0x21, 6, 127, 0x22, 2, 7 };
  const uint8_t a[]   = { 0x40, 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 };   /* 'A' + проміжок */
  const uint8_t one[] = { 0x40, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 };   /* '1' */
  mock_i2c_expect(MI_WRITE, 0x3C, win, sizeof win, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x3C, a, sizeof a, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x3C, one, sizeof one, NULL, 0, ST_OK);
  TEST_ASSERT_EQUAL_INT(ST_OK, ssd1306_text(&d, 2, 6, "a1"));            /* мала → велика */
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_clear_sends_1024_zero_bytes_in_16_byte_chunks(void)
{
  const uint8_t win[] = { 0x00, 0x21, 0, 127, 0x22, 0, 7 };
  uint8_t z[17] = { 0x40 };
  mock_i2c_expect(MI_WRITE, 0x3C, win, sizeof win, NULL, 0, ST_OK);
  for (int i = 0; i < 64; i++) mock_i2c_expect(MI_WRITE, 0x3C, z, 17, NULL, 0, ST_OK);
  TEST_ASSERT_EQUAL_INT(ST_OK, ssd1306_clear(&d));
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_text_stops_at_right_edge(void)
{
  /* col 120: поміщається рівно 1 символ (120+6 ≤ 128), другий — ні */
  const uint8_t win[] = { 0x00, 0x21, 120, 127, 0x22, 0, 7 };
  const uint8_t x[]   = { 0x40, 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 };
  mock_i2c_expect(MI_WRITE, 0x3C, win, sizeof win, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x3C, x, sizeof x, NULL, 0, ST_OK);
  TEST_ASSERT_EQUAL_INT(ST_OK, ssd1306_text(&d, 0, 120, "XY"));
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_bad_arguments(void)
{
  TEST_ASSERT_EQUAL_INT(ST_EPARAM, ssd1306_text(&d, 8, 0, "A"));
  TEST_ASSERT_EQUAL_INT(ST_EPARAM, ssd1306_text(&d, 0, 128, "A"));
}

static void test_nack_from_absent_display(void)
{
  stub_i2c_set(ST_ENACK);
  ssd1306_setup(&d, &stub_i2c_bus, 0x3D);
  TEST_ASSERT_EQUAL_INT(ST_ENACK, ssd1306_probe(&d));
  TEST_ASSERT_EQUAL_INT(ST_ENACK, ssd1306_init(&d));
  TEST_ASSERT_EQUAL_INT(ST_ENACK, ssd1306_clear(&d));
  TEST_ASSERT_EQUAL_INT(ST_ENACK, ssd1306_text(&d, 0, 0, "A"));
}

static void test_error_in_data_phase_stops_text(void)
{
  const uint8_t win[] = { 0x00, 0x21, 0, 127, 0x22, 0, 7 };
  const uint8_t a[]   = { 0x40, 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 };
  mock_i2c_expect(MI_WRITE, 0x3C, win, sizeof win, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x3C, a, sizeof a, NULL, 0, ST_ETIMEOUT);
  TEST_ASSERT_EQUAL_INT(ST_ETIMEOUT, ssd1306_text(&d, 0, 0, "AB"));
  TEST_ASSERT_NULL(mock_i2c_verify());       /* 'B' вже не надсилався */
}

static void test_font_unknown_maps_to_question_mark(void)
{
  TEST_ASSERT_EQUAL_PTR(font5x7_glyph('?'), font5x7_glyph('\x7F'));
  TEST_ASSERT_EQUAL_PTR(font5x7_glyph('?'), font5x7_glyph('\n'));
  TEST_ASSERT_EQUAL_PTR(font5x7_glyph('Z'), font5x7_glyph('z'));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_probe_is_address_only);
  RUN_TEST(test_init_is_single_command_stream_ending_with_display_on);
  RUN_TEST(test_text_sets_window_then_sends_glyph_data);
  RUN_TEST(test_clear_sends_1024_zero_bytes_in_16_byte_chunks);
  RUN_TEST(test_text_stops_at_right_edge);
  RUN_TEST(test_bad_arguments);
  RUN_TEST(test_nack_from_absent_display);
  RUN_TEST(test_error_in_data_phase_stops_text);
  RUN_TEST(test_font_unknown_maps_to_question_mark);
  return UNITY_END();
}
