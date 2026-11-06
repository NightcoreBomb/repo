/**
 * Тести драйвера MPU-6050 (Devices/mpu6050.c) на хості.
 */
#include <string.h>
#include "unity.h"
#include "mpu6050.h"
#include "mock_i2c.h"
#include "fake_mpu6050.h"
#include "stub_i2c.h"
#include "vectors/capture_vectors.h"

static mpu6050_t d;

void setUp(void) { mock_i2c_reset(); fake_mpu_reset(); }
void tearDown(void) { }

/* ---------- перетворення сирих даних у фізичні одиниці ---------- */
static void test_accel_scale_per_range(void)
{
  TEST_ASSERT_EQUAL_INT32(16384, mpu6050_accel_lsb_per_g(MPU_ACCEL_2G));   /* PS §6.2 */
  TEST_ASSERT_EQUAL_INT32(8192,  mpu6050_accel_lsb_per_g(MPU_ACCEL_4G));
  TEST_ASSERT_EQUAL_INT32(4096,  mpu6050_accel_lsb_per_g(MPU_ACCEL_8G));
  TEST_ASSERT_EQUAL_INT32(2048,  mpu6050_accel_lsb_per_g(MPU_ACCEL_16G));
  TEST_ASSERT_EQUAL_INT32(1000,  mpu6050_accel_to_mg(16384, MPU_ACCEL_2G));
  TEST_ASSERT_EQUAL_INT32(-1000, mpu6050_accel_to_mg(-16384, MPU_ACCEL_2G));
  TEST_ASSERT_EQUAL_INT32(500,   mpu6050_accel_to_mg(8192, MPU_ACCEL_2G));
  TEST_ASSERT_EQUAL_INT32(2000,  mpu6050_accel_to_mg(16384, MPU_ACCEL_4G));
  TEST_ASSERT_EQUAL_INT32(16000, mpu6050_accel_to_mg(32767, MPU_ACCEL_16G));  /* верх діапазону */
  TEST_ASSERT_EQUAL_INT32(-16000, mpu6050_accel_to_mg(-32768, MPU_ACCEL_16G));
  TEST_ASSERT_EQUAL_INT32(0,     mpu6050_accel_to_mg(8, MPU_ACCEL_2G));       /* 0.49 mg → 0 */
  TEST_ASSERT_EQUAL_INT32(1,     mpu6050_accel_to_mg(9, MPU_ACCEL_2G));       /* 0.55 mg → 1 */
}

static void test_gyro_scale(void)
{
  TEST_ASSERT_EQUAL_INT32(1000,  mpu6050_gyro_to_mdps(131, MPU_GYRO_250));    /* 131 LSB/(°/с) */
  TEST_ASSERT_EQUAL_INT32(-1000, mpu6050_gyro_to_mdps(-131, MPU_GYRO_250));
  TEST_ASSERT_EQUAL_INT32(250130, mpu6050_gyro_to_mdps(32767, MPU_GYRO_250)); /* 32767/131 = 250.13 °/с */
  TEST_ASSERT_EQUAL_INT32(2000,  mpu6050_gyro_to_mdps(131, MPU_GYRO_500));
  TEST_ASSERT_EQUAL_INT32(1997988, mpu6050_gyro_to_mdps(32767, MPU_GYRO_2000));
}

/* RM-MPU-6000A §4.18: Temperature in °C = TEMP_OUT/340 + 36.53 */
static void test_temperature_formula_from_register_map(void)
{
  TEST_ASSERT_EQUAL_INT32(3653, mpu6050_temp_to_cC(0));       /* TEMP_OUT = 0 → 36.53 °C */
  TEST_ASSERT_EQUAL_INT32(3753, mpu6050_temp_to_cC(340));     /* +1 °C на кожні 340 LSB */
  TEST_ASSERT_EQUAL_INT32(2400, mpu6050_temp_to_cC(-4260));   /* модель Wokwi: 24 °C */
  TEST_ASSERT_EQUAL_INT32(2500, mpu6050_temp_to_cC(-3920));   /* (25-36.53)·340 = -3920.2 */
  TEST_ASSERT_EQUAL_INT32(-4000, mpu6050_temp_to_cC(-26020)); /* -40 °C — нижня межа робочого діапазону */
  TEST_ASSERT_EQUAL_INT32(8500, mpu6050_temp_to_cC(16480));   /* +85 °C — верхня */
}

static void test_parse_burst_big_endian_order(void)
{
  /* порядок на шині: AX_H AX_L AY_H AY_L AZ_H AZ_L T_H T_L GX_H GX_L GY_H GY_L GZ_H GZ_L */
  const uint8_t b[14] = { 0x00,0x01, 0xFF,0xFF, 0x40,0x00, 0xEF,0x5C, 0x00,0x83, 0xFF,0x7D, 0x80,0x00 };
  mpu6050_raw_t r;
  mpu6050_parse_burst(b, &r);
  TEST_ASSERT_EQUAL_INT16(1, r.ax);
  TEST_ASSERT_EQUAL_INT16(-1, r.ay);
  TEST_ASSERT_EQUAL_INT16(16384, r.az);
  TEST_ASSERT_EQUAL_INT16(-4260, r.temp);
  TEST_ASSERT_EQUAL_INT16(131, r.gx);
  TEST_ASSERT_EQUAL_INT16(-131, r.gy);
  TEST_ASSERT_EQUAL_INT16(-32768, r.gz);
}

static void test_isqrt_and_norm(void)
{
  TEST_ASSERT_EQUAL_UINT32(0, isqrt_u32(0));
  TEST_ASSERT_EQUAL_UINT32(1000, isqrt_u32(1000000));
  TEST_ASSERT_EQUAL_UINT32(65535, isqrt_u32(0xFFFFFFFFu));
  TEST_ASSERT_EQUAL_UINT32(999, isqrt_u32(999999));          /* floor */
  mpu6050_data_t v = { .ax_mg = 600, .ay_mg = 0, .az_mg = 800 };
  TEST_ASSERT_EQUAL_UINT32(1000, mpu6050_accel_norm_mg(&v));
  v.ax_mg = -16000; v.ay_mg = -16000; v.az_mg = -16000;      /* максимум без переповнення */
  TEST_ASSERT_EQUAL_UINT32(27712, mpu6050_accel_norm_mg(&v));
}

/* ---------- послідовність транзакцій (MOCK) ---------- */
static void test_identify_reads_who_am_i_register(void)
{
  const uint8_t reg = 0x75, val = 0x68;
  mock_i2c_expect(MI_WRITE_READ, 0x68, &reg, 1, &val, 1, ST_OK);
  mpu6050_setup(&d, &mock_i2c_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_identify(&d));
  TEST_ASSERT_EQUAL_HEX8(0x68, d.who_am_i);
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_init_wakes_chip_first_then_configures(void)
{
  const uint8_t w1[] = { 0x6B, 0x01 }, w2[] = { 0x1A, 0x03 }, w3[] = { 0x19, 0x04 },
                w4[] = { 0x1B, 0x00 }, w5[] = { 0x1C, 0x00 }, rb = 0x6B, pwr = 0x01;
  mock_i2c_expect(MI_WRITE, 0x68, w1, 2, NULL, 0, ST_OK);   /* пробудження — ПЕРШИМ */
  mock_i2c_expect(MI_WRITE, 0x68, w2, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w3, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w4, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w5, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE_READ, 0x68, &rb, 1, &pwr, 1, ST_OK);
  mpu6050_setup(&d, &mock_i2c_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_init(&d));
  TEST_ASSERT_NULL(mock_i2c_verify());
  TEST_ASSERT_EQUAL_HEX8(0x01, d.pwr_mgmt_1);
}

static void test_init_encodes_selected_ranges(void)
{
  const uint8_t w1[] = { 0x6B, 0x01 }, w2[] = { 0x1A, 0x03 }, w3[] = { 0x19, 0x04 },
                w4[] = { 0x1B, 0x18 }, w5[] = { 0x1C, 0x08 }, rb = 0x6B, pwr = 0x01;
  mock_i2c_expect(MI_WRITE, 0x68, w1, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w2, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w3, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w4, 2, NULL, 0, ST_OK);   /* FS_SEL=3 → біти [4:3] */
  mock_i2c_expect(MI_WRITE, 0x68, w5, 2, NULL, 0, ST_OK);   /* AFS_SEL=1 */
  mock_i2c_expect(MI_WRITE_READ, 0x68, &rb, 1, &pwr, 1, ST_OK);
  mpu6050_setup(&d, &mock_i2c_bus, 0x68);
  d.gyro_fs = MPU_GYRO_2000; d.accel_fs = MPU_ACCEL_4G;
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_init(&d));
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_read_raw_is_one_14_byte_burst_from_0x3B(void)
{
  const uint8_t reg = 0x3B;
  const uint8_t b[14] = { 0,0, 0,0, 0x40,0, 0xEF,0x5C, 0,0, 0,0, 0,0 };
  mock_i2c_expect(MI_WRITE_READ, 0x68, &reg, 1, b, 14, ST_OK);
  mpu6050_setup(&d, &mock_i2c_bus, 0x68);
  mpu6050_raw_t r;
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_read_raw(&d, &r));
  TEST_ASSERT_NULL(mock_i2c_verify());
  TEST_ASSERT_EQUAL_INT16(16384, r.az);
}

/* ---------- обробка помилок шини (STUB) і неочікуваного ID ---------- */
static void test_nack_propagates_from_every_call(void)
{
  mpu6050_raw_t r;
  stub_i2c_set(ST_ENACK);
  mpu6050_setup(&d, &stub_i2c_bus, 0x69);
  TEST_ASSERT_EQUAL_INT(ST_ENACK, mpu6050_identify(&d));
  TEST_ASSERT_EQUAL_INT(ST_ENACK, mpu6050_init(&d));
  TEST_ASSERT_EQUAL_INT(ST_ENACK, mpu6050_read_raw(&d, &r));
  TEST_ASSERT_EQUAL_HEX8(0x00, d.who_am_i);         /* не перезаписано сміттям */
}

static void test_timeout_propagates(void)
{
  stub_i2c_set(ST_ETIMEOUT);
  mpu6050_setup(&d, &stub_i2c_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_ETIMEOUT, mpu6050_identify(&d));
  TEST_ASSERT_EQUAL_INT(ST_ETIMEOUT, mpu6050_init(&d));
}

static void test_init_fails_on_nack_mid_sequence(void)
{
  const uint8_t w1[] = { 0x6B, 0x01 }, w2[] = { 0x1A, 0x03 };
  mock_i2c_expect(MI_WRITE, 0x68, w1, 2, NULL, 0, ST_OK);
  mock_i2c_expect(MI_WRITE, 0x68, w2, 2, NULL, 0, ST_ENACK);   /* далі драйвер не йде */
  mpu6050_setup(&d, &mock_i2c_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_ENACK, mpu6050_init(&d));
  TEST_ASSERT_NULL(mock_i2c_verify());
}

static void test_unexpected_id_is_reported(void)
{
  fake_mpu.regs[0x75] = 0x70;              /* MPU-6500 / клон з іншим WHO_AM_I */
  mpu6050_setup(&d, &fake_mpu_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_EID, mpu6050_identify(&d));
  TEST_ASSERT_EQUAL_HEX8(0x70, d.who_am_i);
}

/* ---------- інтеграція з FAKE-моделлю чипа ---------- */
static void test_fake_chip_default_wokwi_state_reads_1g(void)
{
  mpu6050_raw_t r; mpu6050_data_t v;
  mpu6050_setup(&d, &fake_mpu_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_identify(&d));
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_init(&d));
  TEST_ASSERT_EQUAL_HEX8(0x01, fake_mpu.regs[0x6B]);     /* чип справді розбуджено */
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_read_raw(&d, &r));
  mpu6050_convert(&d, &r, &v);
  TEST_ASSERT_EQUAL_INT32(1000, v.az_mg);
  TEST_ASSERT_EQUAL_INT32(0, v.ax_mg);
  TEST_ASSERT_INT32_WITHIN(1, 2400, v.temp_cC);
  TEST_ASSERT_EQUAL_UINT32(1000, mpu6050_accel_norm_mg(&v));
}

static void test_fake_chip_sleeping_returns_zeros_if_not_woken(void)
{
  mpu6050_raw_t r;
  mpu6050_setup(&d, &fake_mpu_bus, 0x68);
  TEST_ASSERT_EQUAL_INT(ST_OK, mpu6050_read_raw(&d, &r));   /* без init */
  TEST_ASSERT_EQUAL_INT16(0, r.az);
}

static void test_fake_chip_tilted_45deg(void)
{
  mpu6050_raw_t r; mpu6050_data_t v;
  fake_mpu_set(707, 0, 707, 0, 0, 0, 2400);
  mpu6050_setup(&d, &fake_mpu_bus, 0x68);
  mpu6050_init(&d);
  mpu6050_read_raw(&d, &r);
  mpu6050_convert(&d, &r, &v);
  TEST_ASSERT_INT32_WITHIN(1, 707, v.ax_mg);
  TEST_ASSERT_INT32_WITHIN(2, 1000, (int32_t)mpu6050_accel_norm_mg(&v));
}

/* ---------- вектори з логічного аналізатора ---------- */
static void test_capture_vectors_who_am_i(void)
{
  TEST_ASSERT_TRUE_MESSAGE(CV_WHO_N > 0, "no WHO_AM_I transaction in docs/captures/*.vcd");
  for (int i = 0; i < CV_WHO_N; i++) {
    const uint8_t reg = 0x75;
    mock_i2c_reset();
    mock_i2c_expect(MI_WRITE_READ, 0x68, &reg, 1, &cv_who[i].bus, 1, ST_OK);
    mpu6050_setup(&d, &mock_i2c_bus, 0x68);
    TEST_ASSERT_EQUAL_INT_MESSAGE(ST_OK, mpu6050_identify(&d), cv_who[i].src);
    if (cv_who[i].uart >= 0)   /* драйвер бачить те саме, що прошивка надрукувала в UART */
      TEST_ASSERT_EQUAL_HEX8_MESSAGE(cv_who[i].uart, d.who_am_i, cv_who[i].src);
  }
}

static void test_capture_vectors_burst_match_uart_log(void)
{
  if (CV_BURST_N == 0)
    TEST_IGNORE_MESSAGE("no 'cap i2c' capture in docs/captures/ yet - see docs/captures/README.md");
  for (int i = 0; i < CV_BURST_N; i++) {
    mpu6050_raw_t r; mpu6050_data_t v;
    mpu6050_setup(&d, &mock_i2c_bus, 0x68);
    mpu6050_parse_burst(cv_burst[i].bus, &r);
    mpu6050_convert(&d, &r, &v);
    if (!cv_burst[i].has_uart) continue;
    const int32_t got[7] = { v.ax_mg, v.ay_mg, v.az_mg, v.temp_cC, v.gx_mdps, v.gy_mdps, v.gz_mdps };
    TEST_ASSERT_EQUAL_INT32_ARRAY_MESSAGE(cv_burst[i].uart, got, 7, cv_burst[i].src);
  }
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_accel_scale_per_range);
  RUN_TEST(test_gyro_scale);
  RUN_TEST(test_temperature_formula_from_register_map);
  RUN_TEST(test_parse_burst_big_endian_order);
  RUN_TEST(test_isqrt_and_norm);
  RUN_TEST(test_identify_reads_who_am_i_register);
  RUN_TEST(test_init_wakes_chip_first_then_configures);
  RUN_TEST(test_init_encodes_selected_ranges);
  RUN_TEST(test_read_raw_is_one_14_byte_burst_from_0x3B);
  RUN_TEST(test_nack_propagates_from_every_call);
  RUN_TEST(test_timeout_propagates);
  RUN_TEST(test_init_fails_on_nack_mid_sequence);
  RUN_TEST(test_unexpected_id_is_reported);
  RUN_TEST(test_fake_chip_default_wokwi_state_reads_1g);
  RUN_TEST(test_fake_chip_sleeping_returns_zeros_if_not_woken);
  RUN_TEST(test_fake_chip_tilted_45deg);
  RUN_TEST(test_capture_vectors_who_am_i);
  RUN_TEST(test_capture_vectors_burst_match_uart_log);
  return UNITY_END();
}
