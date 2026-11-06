/**
 * Тести прикладного шару (App/) повністю на хості: шина I²C — fake MPU-6050
 * (+ ACK від OLED), SPI — fake microSD, сервіси платформи — fake_plat.
 * Перевіряється формат self-test (його розбирають скрипти) і поведінка
 * при відмовах (FR-12).
 */
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "app.h"
#include "selftest.h"
#include "capture.h"
#include "fake_mpu6050.h"
#include "fake_sd.h"
#include "fake_plat.h"
#include "spi_log.h"

static app_t a;
static char v[48];

void setUp(void)
{
  fake_mpu_reset(); fake_sd_reset(); spi_log_reset(&fake_sd_dev); fake_plat_reset();
  app_init(&a, &fake_mpu_bus, &spi_log_bus);
}
void tearDown(void) { }

static long kv(const char *line, const char *k)
{
  TEST_ASSERT_NOT_NULL_MESSAGE(line, "log line not found");
  TEST_ASSERT_NOT_NULL_MESSAGE(fp_kv(line, k, v, sizeof v), k);
  return strtol(v, NULL, 0);
}

static void test_selftest_all_pass_on_default_wokwi_state(void)
{
  st_summary_t s = selftest_run(&a);
  TEST_ASSERT_EQUAL_INT(6, s.pass);
  TEST_ASSERT_EQUAL_INT(0, s.fail);
  TEST_ASSERT_NOT_NULL(fp_find("ST BEGIN fw=LR2 board=HOST-FAKE sysclk=48000000"));
  const char *imu = fp_find("ST imu PASS");
  TEST_ASSERT_EQUAL_INT(0x68, kv(imu, "id"));
  TEST_ASSERT_EQUAL_INT(1000, kv(imu, "az"));
  TEST_ASSERT_EQUAL_INT(1000, kv(imu, "an"));
  TEST_ASSERT_INT_WITHIN(1, 2400, kv(imu, "t_cC"));
  TEST_ASSERT_EQUAL_INT(1, kv(imu, "c_norm"));
  TEST_ASSERT_NOT_NULL(fp_find("ST oled PASS addr=0x3C"));
  const char *sd = fp_find("ST sd PASS");
  TEST_ASSERT_NOT_NULL(sd);
  TEST_ASSERT_EQUAL_STRING("SDHC", fp_kv(sd, "type", v, sizeof v));
  TEST_ASSERT_EQUAL_INT(0x55AA, kv(sd, "sig"));
  TEST_ASSERT_NOT_NULL(fp_find("ST servo PASS pan_us=1500 tilt_us=1500"));
  TEST_ASSERT_NOT_NULL(fp_find("ST pot PASS raw=2048 mv=1650"));
  TEST_ASSERT_NOT_NULL(fp_find("ST btn PASS level=1"));
  TEST_ASSERT_NOT_NULL(fp_find("ST END pass=6 fail=0"));
  TEST_ASSERT_TRUE(a.imu_ok && a.oled_ok && a.sd_ok);
}

/* FR-12: IMU відсутній → повідомлення, серво в центрі, без зависання */
static void test_imu_absent_reported_and_servos_centered(void)
{
  fake_mpu.present = 0;
  unsigned oled_before = fake_mpu.oled_writes;
  st_summary_t s = selftest_run(&a);
  TEST_ASSERT_EQUAL_INT(1, s.fail);
  TEST_ASSERT_NOT_NULL(fp_find("ST imu FAIL step=identify err=NACK"));
  TEST_ASSERT_EQUAL_UINT16(1500, fp_servo[0]);
  TEST_ASSERT_EQUAL_UINT16(1500, fp_servo[1]);
  TEST_ASSERT_TRUE(fake_mpu.oled_writes > oled_before);   /* «IMU NOT FOUND» на OLED */
  TEST_ASSERT_NOT_NULL(fp_find("ST END pass=5 fail=1"));
}

static void test_imu_wrong_magnitude_fails_plausibility(void)
{
  fake_mpu_set(0, 0, 1200, 0, 0, 0, 2400);            /* 1.2 g у спокої — неправдоподібно */
  selftest_imu(&a);
  const char *l = fp_find("ST imu FAIL");
  TEST_ASSERT_EQUAL_INT(0, kv(l, "c_norm"));
  TEST_ASSERT_EQUAL_INT(1, kv(l, "c_gyro"));
}

static void test_imu_within_5_percent_passes(void)
{
  fake_mpu_set(0, 0, 1045, 0, 0, 0, 2400);
  TEST_ASSERT_TRUE(selftest_imu(&a));
  fake_plat_reset();
  fake_mpu_set(0, 0, 940, 0, 0, 0, 2400);             /* −6 % */
  TEST_ASSERT_FALSE(selftest_imu(&a));
}

static void test_imu_gyro_and_temperature_limits(void)
{
  fake_mpu_set(0, 0, 1000, 30000, 0, 0, 2400);        /* 30 °/с у спокої */
  TEST_ASSERT_FALSE(selftest_imu(&a));
  TEST_ASSERT_EQUAL_INT(0, kv(fp_find("ST imu FAIL"), "c_gyro"));
  fake_plat_reset();
  fake_mpu_set(0, 0, 1000, 0, 0, 0, 7000);            /* 70 °C — не кімнатна */
  TEST_ASSERT_FALSE(selftest_imu(&a));
  TEST_ASSERT_EQUAL_INT(0, kv(fp_find("ST imu FAIL"), "c_temp"));
}

static void test_imu_stuck_bus_detected(void)
{
  for (int r = 0x3B; r <= 0x48; r++) fake_mpu.regs[r] = 0xFF;   /* «SDA висить» */
  TEST_ASSERT_FALSE(selftest_imu(&a));
  TEST_ASSERT_EQUAL_INT(0, kv(fp_find("ST imu FAIL"), "c_live"));
}

static void test_imu_bus_timeout_on_read(void)
{
  TEST_ASSERT_TRUE(selftest_imu(&a));
  fake_plat_reset();
  fake_mpu.force_err = ST_ETIMEOUT;
  TEST_ASSERT_FALSE(selftest_imu(&a));
  TEST_ASSERT_NOT_NULL(fp_find("ST imu FAIL step=identify err=TIMEOUT"));
}

static void test_imuchk_command_matches_set_values(void)
{
  selftest_run(&a);
  fake_mpu_set(1000, 0, 0, 0, 0, 0, 3000);
  char cmd[] = "imuchk 1000 0 0 0 0 0 3000";
  fake_plat_reset();
  app_handle_line(&a, cmd);
  TEST_ASSERT_NOT_NULL(fp_find("IMUCHK PASS ax=1000"));
  char cmd2[] = "imuchk 0 0 1000";                      /* очікування не збігається */
  fake_plat_reset();
  app_handle_line(&a, cmd2);
  TEST_ASSERT_NOT_NULL(fp_find("IMUCHK FAIL"));
}

static void test_imuchk_gyro(void)
{
  selftest_run(&a);
  fake_mpu_set(0, 0, 1000, 0, 90000, 0, 2400);
  char cmd[] = "imuchk 0 0 1000 0 90000 0 2400";
  fake_plat_reset();
  app_handle_line(&a, cmd);
  TEST_ASSERT_NOT_NULL(fp_find("IMUCHK PASS"));
}

static void test_sd_selftest_is_non_destructive(void)
{
  memset(fake_sd.mem[1], 0x5A, 512);
  TEST_ASSERT_TRUE(selftest_sd(&a));
  for (int i = 0; i < 512; i++) TEST_ASSERT_EQUAL_HEX8(0x5A, fake_sd.mem[1][i]);
  TEST_ASSERT_EQUAL_STRING("OK", fp_kv(fp_find("ST sd PASS"), "rw", v, sizeof v));
}

/* Дефект, знайдений тестом: якщо оригінал блоку не прочитано, «відновлення»
   записувало б у карту вміст буфера від попереднього виклику. */
static void test_sd_no_write_if_original_block_unreadable(void)
{
  memset(fake_sd.mem[1], 0x5A, 512);
  fake_sd.fail_read_blk = 1;
  TEST_ASSERT_FALSE(selftest_sd(&a));
  TEST_ASSERT_EQUAL_INT(0, fake_sd.writes);
  TEST_ASSERT_EQUAL_HEX8(0x5A, fake_sd.mem[1][0]);
  TEST_ASSERT_EQUAL_STRING("rd", fp_kv(fp_find("ST sd FAIL"), "step", v, sizeof v));
}

static void test_sd_absent(void)
{
  fake_sd.absent = true;
  TEST_ASSERT_FALSE(selftest_sd(&a));
  TEST_ASSERT_NOT_NULL(fp_find("ST sd FAIL step=init err=NORESP"));
}

static void test_sd_write_error_reported_and_restore_attempted(void)
{
  fake_sd.reject_write = true;
  TEST_ASSERT_FALSE(selftest_sd(&a));
  const char *l = fp_find("ST sd FAIL");
  TEST_ASSERT_EQUAL_STRING("VERIFY", fp_kv(l, "rw", v, sizeof v));
  TEST_ASSERT_EQUAL_STRING("wr", fp_kv(l, "step", v, sizeof v));
}

static void test_pot_and_button_failures(void)
{
  fp_pot = -1; fp_btn_level = 0;
  TEST_ASSERT_FALSE(selftest_pot());
  TEST_ASSERT_FALSE(selftest_btn());
  TEST_ASSERT_NOT_NULL(fp_find("ST pot FAIL raw=-1"));
  TEST_ASSERT_NOT_NULL(fp_find("ST btn FAIL level=0"));
}

static void test_capture_i2c_prints_bus_bytes_and_conversion(void)
{
  selftest_run(&a);
  fake_plat_reset();
  TEST_ASSERT_TRUE(capture_run(&a, "i2c"));
  TEST_ASSERT_EQUAL_INT(2, fp_mark_edges);                 /* MARK ↑ і ↓ рівно один раз */
  TEST_ASSERT_NOT_NULL(fp_find("CAP i2c probe68=OK probe3C=OK who=0x68(OK) burst=OK"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP i2c burst3B: 00 00 00 00 40 00 EF 5C 00 00 00 00 00 00"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP i2c conv ax=0 ay=0 az=1000 t_cC=2400"));
}

static void test_capture_nack_anomaly(void)
{
  TEST_ASSERT_TRUE(capture_run(&a, "nack"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP nack addr=0x69 result=NACK"));
}

static void test_capture_rs_switches_mode_and_restores(void)
{
  selftest_run(&a);
  capture_run(&a, "rs");
  TEST_ASSERT_FALSE(fp_restart);
  TEST_ASSERT_NOT_NULL(fp_find("CAP rs who=0x68(OK) burst=OK"));
}

static void test_capture_all_and_unknown(void)
{
  TEST_ASSERT_TRUE(capture_run(&a, "all"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP spi init=OK type=SDHC"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP pwm pan=1000us->2000us"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP adc n=20 period_ms=5"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP uart U=0x55"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP all end"));
  TEST_ASSERT_EQUAL_UINT16(1500, fp_servo[0]);              /* серво повернуто в центр */
  TEST_ASSERT_FALSE(capture_run(&a, "bogus"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP bogus unknown"));
  TEST_ASSERT_TRUE(capture_run(&a, "spiw"));
  TEST_ASSERT_NOT_NULL(fp_find("CAP spiw blk=1 write=OK"));
}

static void test_console_commands(void)
{
  char c1[] = "scan", c2[] = "servo 100 3000", c3[] = "status", c4[] = "nope", c5[] = "imu",
       c6[] = "help", c7[] = "", c8[] = "cap uart", c9[] = "selftest";
  app_handle_line(&a, c1);
  TEST_ASSERT_NOT_NULL(fp_find("SCAN 0x3C 0x68 n=2"));
  app_handle_line(&a, c2);
  TEST_ASSERT_NOT_NULL(fp_find("SERVO pan_us=500 tilt_us=2500"));  /* обрізано (FR-05) */
  app_handle_line(&a, c3);
  TEST_ASSERT_NOT_NULL(fp_find("STATUS imu=0"));
  app_handle_line(&a, c4);
  TEST_ASSERT_NOT_NULL(fp_find("ERR unknown 'nope'"));
  app_handle_line(&a, c5);
  TEST_ASSERT_NOT_NULL(fp_find("IMU raw="));
  app_handle_line(&a, c6);
  app_handle_line(&a, c7);
  app_handle_line(&a, c8);
  TEST_ASSERT_NOT_NULL(fp_find("CAP uart end"));
  app_handle_line(&a, c9);
  TEST_ASSERT_NOT_NULL(fp_find("ST END pass=6"));
}

static void test_imu_command_reports_bus_error(void)
{
  fake_mpu.present = 0;
  char c[] = "imu", k[] = "imuchk 0 0 1000";
  app_handle_line(&a, c);
  TEST_ASSERT_NOT_NULL(fp_find("IMU err=NACK"));
  app_handle_line(&a, k);
  TEST_ASSERT_NOT_NULL(fp_find("IMUCHK FAIL err=NACK"));
}

static void test_poll_tick_button_and_console(void)
{
  selftest_run(&a);
  fake_plat_reset();
  fp_btn_presses = 2;
  fp_console_line = "status";
  app_poll(&a);
  TEST_ASSERT_NOT_NULL(fp_find("STATUS imu=1 oled=1 sd=1"));
  TEST_ASSERT_NOT_NULL(fp_find("[btn ] pressed, total = 2"));
  TEST_ASSERT_NOT_NULL(fp_find("tick 1 |"));
  TEST_ASSERT_NOT_NULL(strstr(fp_find("tick 1"), "az=1000 mg"));
  app_poll(&a);                                               /* до наступного тіку ще не час */
  TEST_ASSERT_NULL(fp_find("tick 2"));
  fp_now += APP_TICK_MS;
  app_poll(&a);
  TEST_ASSERT_NOT_NULL(fp_find("tick 2"));
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_selftest_all_pass_on_default_wokwi_state);
  RUN_TEST(test_imu_absent_reported_and_servos_centered);
  RUN_TEST(test_imu_wrong_magnitude_fails_plausibility);
  RUN_TEST(test_imu_within_5_percent_passes);
  RUN_TEST(test_imu_gyro_and_temperature_limits);
  RUN_TEST(test_imu_stuck_bus_detected);
  RUN_TEST(test_imu_bus_timeout_on_read);
  RUN_TEST(test_imuchk_command_matches_set_values);
  RUN_TEST(test_imuchk_gyro);
  RUN_TEST(test_sd_selftest_is_non_destructive);
  RUN_TEST(test_sd_no_write_if_original_block_unreadable);
  RUN_TEST(test_sd_absent);
  RUN_TEST(test_sd_write_error_reported_and_restore_attempted);
  RUN_TEST(test_pot_and_button_failures);
  RUN_TEST(test_capture_i2c_prints_bus_bytes_and_conversion);
  RUN_TEST(test_capture_nack_anomaly);
  RUN_TEST(test_capture_rs_switches_mode_and_restores);
  RUN_TEST(test_capture_all_and_unknown);
  RUN_TEST(test_console_commands);
  RUN_TEST(test_imu_command_reports_bus_error);
  RUN_TEST(test_poll_tick_button_and_console);
  return UNITY_END();
}
