/** Тести драйвера microSD (SPI mode) з FAKE-картою і журналом SPI-транзакцій. */
#include <string.h>
#include "unity.h"
#include "sdcard.h"
#include "spi_log.h"
#include "fake_sd.h"

static sdcard_t d;
void setUp(void) { fake_sd_reset(); spi_log_reset(&fake_sd_dev); sd_setup(&d, &spi_log_bus); }
void tearDown(void) { }

/* SD Spec 7.2.2: CMD0 з аргументом 0 → CRC 0x95; CMD8 0x1AA → 0x87 */
static void test_crc7_known_frames(void)
{
  uint8_t f[6];
  sd_build_cmd(f, 0, 0);
  TEST_ASSERT_EQUAL_HEX8(0x40, f[0]); TEST_ASSERT_EQUAL_HEX8(0x95, f[5]);
  sd_build_cmd(f, 8, 0x1AA);
  TEST_ASSERT_EQUAL_HEX8(0x48, f[0]); TEST_ASSERT_EQUAL_HEX8(0x87, f[5]);
  sd_build_cmd(f, 17, 0);                   /* CMD17 arg 0 → 0x55 (поширений приклад) */
  TEST_ASSERT_EQUAL_HEX8(0x55, f[5]);
  sd_build_cmd(f, 55, 0);
  TEST_ASSERT_EQUAL_HEX8(0x65, f[5]);
}

static void test_init_sequence_order_and_cs_framing(void)
{
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_init(&d));
  TEST_ASSERT_EQUAL_INT(SD_V2_SDHC, d.type);
  TEST_ASSERT_EQUAL_INT(0, fake_sd.bad_crc_seen);
  /* до першого CS↓ — 80 тактів при CS# high, повільна частота */
  int bytes_before_cs = 0;
  for (int i = 0; i < spi_log_n && spi_log[i].ev != EV_CS_LOW; i++)
    if (spi_log[i].ev == EV_BYTE) { bytes_before_cs++; TEST_ASSERT_EQUAL_HEX8(0xFF, spi_log[i].mosi); }
  TEST_ASSERT_EQUAL_INT(10, bytes_before_cs);
  TEST_ASSERT_EQUAL_INT(EV_SLOW, spi_log[0].ev);
  /* порядок команд: CMD0, CMD8, (CMD55, ACMD41)×4, CMD58 */
  const uint8_t order[] = { 0x40, 0x48, 0x77, 0x69, 0x77, 0x69, 0x77, 0x69, 0x77, 0x69, 0x7A };
  int k = 0;
  for (unsigned i = 0; i < sizeof order; i++) {
    k = spi_find_frame(order[i], k);
    TEST_ASSERT_TRUE_MESSAGE(k >= 0, "command missing or out of order");
    k++;
  }
  TEST_ASSERT_EQUAL_UINT16(4, d.acmd41_tries);
  TEST_ASSERT_TRUE(spi_fast);                                /* після init — робоча частота */
  TEST_ASSERT_FALSE(spi_cs_low);
}

static void test_acmd41_always_prefixed_by_cmd55(void)
{
  sd_init(&d);
  uint8_t prev = 0, b[16];
  for (int k = 0; k < spi_frame_count(); k++) {
    int n = spi_frame(k, b, NULL, sizeof b), i = 0;
    while (i < n && b[i] == 0xFF) i++;
    if (i >= n) continue;
    if (b[i] == 0x69) TEST_ASSERT_EQUAL_HEX8(0x77, prev);     /* ACMD41 ← CMD55 */
    prev = b[i];
  }
}

static void test_sdsc_card_uses_byte_addressing(void)
{
  uint8_t buf[512];
  fake_sd.sdhc = false;
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_init(&d));
  TEST_ASSERT_EQUAL_INT(SD_V2_SDSC, d.type);
  TEST_ASSERT_TRUE(spi_find_frame(0x50, 0) >= 0);            /* CMD16 (SET_BLOCKLEN) */
  int from = spi_frame_count();
  fake_sd.mem[3][0] = 0x3C;
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_read_block(&d, 3, buf));
  uint8_t f[16]; int k = spi_find_frame(0x51, from), i = 0;
  spi_frame(k, f, NULL, 16);
  while (f[i] == 0xFF) i++;
  TEST_ASSERT_EQUAL_HEX8(0x00, f[i + 2]);
  TEST_ASSERT_EQUAL_HEX8(0x06, f[i + 3]);                     /* 3·512 = 0x000600 */
  TEST_ASSERT_EQUAL_HEX8(0x00, f[i + 4]);
  TEST_ASSERT_EQUAL_HEX8(0x3C, buf[0]);
}

static void test_v1_card_detected_by_illegal_cmd8(void)
{
  fake_sd.v1 = true;
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_init(&d));
  TEST_ASSERT_EQUAL_INT(SD_V1_SDSC, d.type);
}

static void test_no_card_gives_noresp(void)
{
  fake_sd.absent = true;
  TEST_ASSERT_EQUAL_INT(ST_ENORESP, sd_init(&d));
  TEST_ASSERT_EQUAL_INT(SD_NONE, d.type);
  uint8_t b[512];
  TEST_ASSERT_EQUAL_INT(ST_EPARAM, sd_read_block(&d, 0, b));
}

static void test_acmd41_never_ready_times_out(void)
{
  fake_sd.acmd41_busy = 1000000;
  TEST_ASSERT_EQUAL_INT(ST_ETIMEOUT, sd_init(&d));
}

static void test_read_block0_signature(void)
{
  uint8_t b[512];
  sd_init(&d);
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_read_block(&d, 0, b));
  TEST_ASSERT_EQUAL_HEX8(0x55, b[510]);
  TEST_ASSERT_EQUAL_HEX8(0xAA, b[511]);
}

static void test_write_then_read_back_byte_exact(void)
{
  uint8_t w[512], r[512];
  for (int i = 0; i < 512; i++) w[i] = (uint8_t)(i * 7 + 1);
  sd_init(&d);
  int from = spi_frame_count();
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_write_block(&d, 5, w));
  /* у кадрі запису: CMD24, далі start token 0xFE і рівно 512 байтів даних */
  uint8_t f[600]; int k = spi_find_frame(0x58, from);
  int n = spi_frame(k, f, NULL, sizeof f);
  int tok = -1;
  int c = 0; while (f[c] == 0xFF) c++;                       /* початок CMD24 */
  for (int i = c + 6; i < n; i++) if (f[i] == 0xFE) { tok = i; break; }
  TEST_ASSERT_TRUE(tok > 0);
  TEST_ASSERT_EQUAL_MEMORY(w, &f[tok + 1], 512);
  TEST_ASSERT_EQUAL_INT(ST_OK, sd_read_block(&d, 5, r));
  TEST_ASSERT_EQUAL_MEMORY(w, r, 512);
}

static void test_write_rejected_by_card(void)
{
  uint8_t w[512] = { 0 };
  sd_init(&d);
  fake_sd.reject_write = true;
  TEST_ASSERT_EQUAL_INT(ST_EVERIFY, sd_write_block(&d, 1, w));
  TEST_ASSERT_FALSE(spi_cs_low);
}

static void test_read_out_of_range_gives_r1_error(void)
{
  uint8_t b[512];
  sd_init(&d);
  TEST_ASSERT_EQUAL_INT(ST_ENORESP, sd_read_block(&d, 100, b));
  TEST_ASSERT_EQUAL_HEX8(0x40, d.last_r1);
}

static void test_type_strings(void)
{
  TEST_ASSERT_EQUAL_STRING("SDHC", sd_type_str(SD_V2_SDHC));
  TEST_ASSERT_EQUAL_STRING("SDSC", sd_type_str(SD_V2_SDSC));
  TEST_ASSERT_EQUAL_STRING("SDv1", sd_type_str(SD_V1_SDSC));
  TEST_ASSERT_EQUAL_STRING("none", sd_type_str(SD_NONE));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_crc7_known_frames);
  RUN_TEST(test_init_sequence_order_and_cs_framing);
  RUN_TEST(test_acmd41_always_prefixed_by_cmd55);
  RUN_TEST(test_sdsc_card_uses_byte_addressing);
  RUN_TEST(test_v1_card_detected_by_illegal_cmd8);
  RUN_TEST(test_no_card_gives_noresp);
  RUN_TEST(test_acmd41_never_ready_times_out);
  RUN_TEST(test_read_block0_signature);
  RUN_TEST(test_write_then_read_back_byte_exact);
  RUN_TEST(test_write_rejected_by_card);
  RUN_TEST(test_read_out_of_range_gives_r1_error);
  RUN_TEST(test_type_strings);
  return UNITY_END();
}
