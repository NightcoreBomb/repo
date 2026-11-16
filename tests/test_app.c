/**
 * Тести прикладного шару (App/) повністю на хості: шина I²C — fake MPU-6050
 * (+ ACK від OLED), SPI — fake microSD (вікно журналу LBA 2048+), сервіси платформи —
 * fake_plat з віртуальним часом (1 такт = 1 мкс).
 *
 * Частина 1 — self-test (bring-up з ЛР2, команда `selftest`).
 * Частина 2 — продукт ЛР3 у super loop: FR-01..FR-12 на рівні логіки.
 */
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "app.h"
#include "selftest.h"
#include "fake_mpu6050.h"
#include "fake_sd.h"
#include "fake_plat.h"
#include "plat.h"
#include "servo.h"
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

static void log_clear(void) { fp_log[0] = 0; fake_plat_log_rewind(); }

/* Віртуальний super loop: ітерація ≈ 50 мкс + SysTick кожну мс */
static void run_ms(uint32_t ms)
{
  uint32_t end = fp_cyc + ms * 1000u, next_tick = (fp_cyc / 1000u + 1u) * 1000u;
  while ((int32_t)(fp_cyc - end) < 0) {
    app_poll(&a);
    fp_cyc += 50;
    while ((int32_t)(fp_cyc - next_tick) >= 0) { app_tick_1ms(); next_tick += 1000u; }
  }
}

static void press(void)
{
  fp_btn_first_edge = fp_cyc;
  fp_btn_level = 0; run_ms(70);
  fp_btn_level = 1; run_ms(70);
}

static void start(void)
{
  app_start(&a, true);
  log_clear();
}

/* pitch θ: ax = −sin θ, az = cos θ (мг) */
static void tilt_pitch(int deg_x10)
{
  static const int32_t s10[] = { 0, 174, 342, 500, 643, 766, 866, 940, 985, 1000 }; /* sin 0..90 крок 10° */
  int d = deg_x10 < 0 ? -deg_x10 : deg_x10;
  int32_t sn = s10[d / 100], cs = s10[9 - d / 100];
  fake_mpu_set(deg_x10 < 0 ? sn : -sn, 0, cs, 0, 0, 0, 2400);
}

static void test_selftest_all_pass_on_default_wokwi_state(void)
{
  st_summary_t s = selftest_run(&a);
  TEST_ASSERT_EQUAL_INT(6, s.pass);
  TEST_ASSERT_EQUAL_INT(0, s.fail);
  TEST_ASSERT_NOT_NULL(fp_find("ST BEGIN fw=LR3 board=HOST-FAKE sysclk=48000000"));
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

static void test_sd_selftest_is_non_destructive(void)
{
  memset(fake_sd.mem[1], 0x5A, 512);
  TEST_ASSERT_TRUE(selftest_sd(&a));
  for (int i = 0; i < 512; i++) TEST_ASSERT_EQUAL_HEX8(0x5A, fake_sd.mem[1][i]);
  TEST_ASSERT_EQUAL_STRING("OK", fp_kv(fp_find("ST sd PASS"), "rw", v, sizeof v));
}

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

/* ======================= ЛР3: продукт у super loop ======================= */

static void test_start_checks_and_ready(void)
{
  app_start(&a, true);
  TEST_ASSERT_NOT_NULL(fp_find("CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK"));
  TEST_ASSERT_NOT_NULL(fp_find("DIR NONE blocks=0"));
  TEST_ASSERT_NOT_NULL(fp_find("READY"));
  TEST_ASSERT_NOT_NULL(strstr(fp_log, "LR3 super-loop"));
}

/* FR-12: без IMU — повідомлення, серво в центрі, цикл не зависає і не рухає серво */
static void test_start_without_imu_fr12(void)
{
  fake_mpu.present = 0;
  app_start(&a, true);
  TEST_ASSERT_NOT_NULL(fp_find("CHK imu=NACK"));
  TEST_ASSERT_NOT_NULL(fp_find("ERR IMU not found"));
  run_ms(300);
  TEST_ASSERT_EQUAL_UINT16(1500, fp_servo[0]);
  TEST_ASSERT_EQUAL_UINT16(1500, fp_servo[1]);
  TEST_ASSERT_TRUE(fake_mpu.oled_writes > 0);
}

/* FR-01: 200 Гц, джитер у межах одного кроку віртуального циклу */
static void test_imu_task_200hz(void)
{
  start();
  run_ms(1000);
  TEST_ASSERT_INT_WITHIN(2, 200, a.imu_samples);
  TEST_ASSERT_EQUAL_UINT32(0, a.jit_imu.missed);
  TEST_ASSERT_TRUE(a.jit_imu.jmax_us <= 100);
  TEST_ASSERT_TRUE(a.st[T_IMU].n >= 198);
}

/* FR-02/03: нахил 30° → tilt-серво 120° (1833 мкс), pan у центрі */
static void test_live_servo_follows_tilt(void)
{
  fp_pot = 0;                                        /* dead-zone 0 */
  start();
  tilt_pitch(300);
  run_ms(1500);
  TEST_ASSERT_INT_WITHIN(300, 3000, a.pitch_cd);     /* ≤ 3° (FR-02) */
  TEST_ASSERT_INT_WITHIN(15, 1833, fp_servo[1]);
  TEST_ASSERT_INT_WITHIN(5, 1500, fp_servo[0]);
  TEST_ASSERT_TRUE(a.lat[L_CTRL].n > 0);
}

/* FR-04: dead-zone 10° (потенціометр на максимум) ігнорує нахил 5°… */
static void test_deadzone_from_pot_fr04(void)
{
  fp_pot = 4095;
  start();
  fake_mpu_set(-87, 0, 996, 0, 0, 0, 2400);          /* ≈ 5° */
  run_ms(1000);
  TEST_ASSERT_EQUAL_INT(1000, a.dz_cd);
  TEST_ASSERT_EQUAL_UINT16(1500, fp_servo[1]);
  tilt_pitch(300);                                   /* …але не 30° */
  run_ms(1000);
  TEST_ASSERT_TRUE(fp_servo[1] > 1700);
}

/* FR-05: підстановка ±120° — імпульс лишається 500..2500 мкс */
static void test_angle_override_clamped_fr05(void)
{
  fp_pot = 0;
  start();
  char c[] = "angle 12000 -12000";
  app_handle_line(&a, c);
  run_ms(1500);
  TEST_ASSERT_EQUAL_UINT16(2500, fp_servo[1]);
  TEST_ASSERT_EQUAL_UINT16(500, fp_servo[0]);
  char off[] = "angle off";
  app_handle_line(&a, off);
  TEST_ASSERT_FALSE(a.ovr);
}

/* FR-06..FR-08: Live → Record → Playback → Live кнопкою; темп відтворення */
static void test_record_playback_roundtrip(void)
{
  fp_pot = 0;
  start();
  press();
  TEST_ASSERT_EQUAL_INT(MODE_REC, a.mode);
  tilt_pitch(300);
  run_ms(3000);
  tilt_pitch(-200);
  run_ms(3000);
  uint16_t last = fp_servo[1];
  press();                                           /* → PLAY: блоки + каталог на SD */
  TEST_ASSERT_EQUAL_INT(MODE_PLAY, a.mode);
  const char *st = fp_find("REC stop");
  TEST_ASSERT_INT_WITHIN(5, 307, kv(st, "records"));  /* 6.14 с @50 Гц */
  TEST_ASSERT_EQUAL_INT(2, kv(st, "blocks"));
  TEST_ASSERT_EQUAL_INT(0, kv(st, "gaps"));
  TEST_ASSERT_TRUE(a.dir_ok);
  run_ms(1000);
  TEST_ASSERT_TRUE(fp_servo[1] > 1750);              /* відтворюється перша частина (30°) */
  run_ms(6000);
  const char *d = fp_find("PLAY done");
  TEST_ASSERT_NOT_NULL(d);
  long dur = kv(d, "duration_ms"), rec = kv(d, "rec_ms");
  TEST_ASSERT_TRUE(labs(dur - rec) * 100 <= rec * 2);   /* FR-08: ≤ 2 % */
  TEST_ASSERT_INT_WITHIN(12, last, fp_servo[1]);     /* кінцеве положення збігається */
  press();
  TEST_ASSERT_EQUAL_INT(MODE_LIVE, a.mode);
  TEST_ASSERT_EQUAL_UINT32(3, a.lat[L_BTN_DB].n);
}

/* FR-09: журнал переживає перезапуск; зіпсований блок виявляє CRC */
static void test_recording_survives_restart_fr09(void)
{
  start();
  char r[] = "mode rec", p[] = "mode play";
  app_handle_line(&a, r);
  run_ms(2000);
  app_handle_line(&a, p);
  uint32_t recs = a.dir.records;
  app_t b;
  app_init(&b, &fake_mpu_bus, &spi_log_bus);
  log_clear();
  app_start(&b, true);
  const char *dir = fp_find("DIR OK");
  TEST_ASSERT_NOT_NULL(dir);
  TEST_ASSERT_EQUAL_UINT32(recs, kv(dir, "records"));
  fake_sd.hi[1][100] ^= 0xFF;                       /* біт-флип у першому блоці даних */
  char pl[] = "mode play";
  app_handle_line(&b, pl);
  TEST_ASSERT_NOT_NULL(fp_find("ERR play blk=0 crc"));
}

/* FR-07: затримка циклу > 25 мс — пропуск у журналі (обмеження super loop) */
static void test_record_gap_counted(void)
{
  start();
  char r[] = "mode rec";
  app_handle_line(&a, r);
  run_ms(200);
  fp_advance_ms(60);                                 /* довга блокувальна операція */
  run_ms(200);
  TEST_ASSERT_TRUE(a.rec_gaps >= 1);
  TEST_ASSERT_TRUE(a.rec_max_int >= 60);
}

static void test_sd_absent_blocks_record(void)
{
  fake_sd.absent = true;
  app_start(&a, true);
  TEST_ASSERT_NOT_NULL(fp_find("ERR SD not ready"));
  char r[] = "mode rec";
  app_handle_line(&a, r);
  TEST_ASSERT_NOT_NULL(fp_find("ERR SD not ready, mode stays LIVE"));
  TEST_ASSERT_EQUAL_INT(MODE_LIVE, a.mode);
}

/* FR-11: телеметрія t,pitch,roll,pan,tilt,mode і команди status/mode/calib */
static void test_telemetry_and_commands_fr11(void)
{
  start();
  run_ms(250);
  TEST_ASSERT_TRUE(strstr(fp_log, ",1500,1500,LIVE\r\n") != NULL);
  char s[] = "status", m[] = "mode", c[] = "calib", t[] = "telem 0", h[] = "help", u[] = "bogus";
  app_handle_line(&a, s);
  TEST_ASSERT_NOT_NULL(fp_find("STATUS mode=LIVE"));
  app_handle_line(&a, m);
  TEST_ASSERT_NOT_NULL(fp_find("MODE REC"));
  fake_mpu_set(0, 0, 1000, 1000, -2000, 3000, 2400);
  app_handle_line(&a, c);
  const char *cl = fp_find("CALIB n=200");
  TEST_ASSERT_NOT_NULL(cl);
  TEST_ASSERT_INT_WITHIN(10, -2000, a.att.bias_mdps[1]);
  TEST_ASSERT_NOT_NULL(fp_find("EVT t_us="));
  app_handle_line(&a, t);
  log_clear();
  run_ms(300);
  TEST_ASSERT_NULL(strstr(fp_log, ",REC\r\n"));
  app_handle_line(&a, h);
  TEST_ASSERT_NOT_NULL(fp_find("cmds:"));
  app_handle_line(&a, u);
  TEST_ASSERT_NOT_NULL(fp_find("ERR unknown 'bogus'"));
}

/* Вимірювальний вивід (формат розбирає tools/lr3_metrics.py) */
static void test_stats_format_and_latencies(void)
{
  start();
  run_ms(500);
  fp_console_eol = fp_cyc - 300;
  fp_console_line = "status";
  run_ms(10);
  press();
  log_clear();
  char s[] = "stats", hs[] = "hist", r[] = "stats reset";
  app_handle_line(&a, s);
  TEST_ASSERT_NOT_NULL(fp_find("STAT BEGIN fw="));
  TEST_ASSERT_NOT_NULL(fp_find("STAT name=imu n="));
  TEST_ASSERT_NOT_NULL(fp_find("STAT name=loop n="));
  TEST_ASSERT_NOT_NULL(fp_find("JIT name=imu period_us=5000"));
  const char *lc = fp_find("LAT name=cmd");
  TEST_ASSERT_TRUE(kv(lc, "min_us") >= 300);
  TEST_ASSERT_NOT_NULL(fp_find("LAT name=btn_db n=1"));
  TEST_ASSERT_NOT_NULL(fp_find("LOAD last_pct="));
  TEST_ASSERT_NOT_NULL(fp_find("MEM stack_used=640"));
  TEST_ASSERT_NOT_NULL(fp_find("STAT END"));
  app_handle_line(&a, hs);
  TEST_ASSERT_NOT_NULL(fp_find("HIST name=imu lt50="));
  app_handle_line(&a, r);
  TEST_ASSERT_EQUAL_UINT32(0, a.st[T_IMU].n);
}

/* FR-10: OLED ≥ 5 кадрів/с у всіх трьох способах оновлення */
static void test_oled_modes_frame_rate_fr10(void)
{
  start();
  for (int m = 0; m <= 2; m++) {
    char c[12] = "oled 0";
    c[5] = (char)('0' + m);
    app_handle_line(&a, c);
    uint32_t f0 = a.oled_frames;
    run_ms(2000);
    TEST_ASSERT_INT_WITHIN(1, 10, a.oled_frames - f0);
  }
}

/* Відмова IMU під час роботи: помилки рахуються, після відновлення — знову дані */
static void test_imu_failure_and_recovery(void)
{
  start();
  run_ms(100);
  fake_mpu.force_err = ST_ETIMEOUT;
  run_ms(100);
  TEST_ASSERT_TRUE(a.imu_errors >= 15);
  TEST_ASSERT_NOT_NULL(strstr(fp_log, "name=imu_err"));
  fake_mpu.force_err = 0;
  run_ms(100);
  TEST_ASSERT_NOT_NULL(strstr(fp_log, "name=imu_ok"));
  TEST_ASSERT_EQUAL_INT(ST_OK, a.imu_status);
  char f1[] = "fault i2c 1", f0[] = "fault i2c 0";
  app_handle_line(&a, f1);
  TEST_ASSERT_EQUAL_INT(1, fp_i2c_fault);
  app_handle_line(&a, f0);
  TEST_ASSERT_EQUAL_INT(0, fp_i2c_fault);
}

/* Довга операція dump: hex-дамп журналу, PROBE_LONG ↑↓ */
static void test_dump_and_misc_commands(void)
{
  start();
  char r[] = "mode rec", p[] = "mode play", d[] = "dump", d2[] = "dump", n[] = "note S3 phase";
  app_handle_line(&a, r);
  run_ms(500);
  app_handle_line(&a, d2);                           /* у REC буфер блоку зайнятий записом */
  TEST_ASSERT_NOT_NULL(fp_find("ERR dump busy: recording"));
  app_handle_line(&a, n);
  TEST_ASSERT_NOT_NULL(fp_find("NOTE S3 phase\r\n"));
  app_handle_line(&a, p);
  log_clear();
  int e0 = fp_probe_edges[PROBE_LONG];
  app_handle_line(&a, d);
  TEST_ASSERT_NOT_NULL(fp_find("DUMP blocks=1"));
  TEST_ASSERT_EQUAL_INT(16, fp_count("D 0 "));
  TEST_ASSERT_NOT_NULL(fp_find("D 0 000 544A"));     /* magic 'TJ' little-endian */
  TEST_ASSERT_TRUE(fp_probe_edges[PROBE_LONG] - e0 >= 2);
  char rt[] = "rate 100", ld[] = "load 300", ev[] = "evt off", ol[] = "oled 9";
  app_handle_line(&a, rt);
  TEST_ASSERT_EQUAL_UINT32(10000, a.imu_period_us);
  app_handle_line(&a, ld);
  TEST_ASSERT_EQUAL_UINT32(300, a.busy_us);
  app_handle_line(&a, ev);
  TEST_ASSERT_FALSE(a.evt);
  app_handle_line(&a, ol);
  TEST_ASSERT_EQUAL_INT(2, a.oled_mode);
  run_ms(50);
  char st[] = "selftest";
  app_handle_line(&a, st);
  TEST_ASSERT_NOT_NULL(fp_find("ST END"));
}

/* П-18: dump під час відтворення перезаписує буфер блоку — відтворення має продовжитись
 * з того самого запису, а не перескочити на наступний блок. */
static void test_dump_during_play_keeps_all_records(void)
{
  fp_pot = 0;
  start();
  press();
  tilt_pitch(250);
  run_ms(3000);
  press();                                           /* → PLAY */
  long rec = kv(fp_find("REC stop"), "records");
  run_ms(1000);
  char d[] = "dump";
  app_handle_line(&a, d);
  TEST_ASSERT_NOT_NULL(fp_find("DUMP end"));
  run_ms(4000);
  const char *pd = fp_find("PLAY done");
  TEST_ASSERT_NOT_NULL(pd);
  TEST_ASSERT_EQUAL_INT(rec, kv(pd, "records"));
}

/* П-19: мітка першого фронту береться на момент debounce, а не на момент реакції */
static void test_btn_edge_latched_at_debounce(void)
{
  start();
  fp_btn_first_edge = fp_cyc;
  fp_btn_level = 0;
  for (int i = 0; i < 60; i++) { fp_advance_ms(1); app_tick_1ms(); }   /* цикл «зайнятий» */
  fp_btn_level = 1;
  fp_btn_first_edge = fp_cyc;                        /* «брязкіт відпускання» */
  for (int i = 0; i < 40; i++) { fp_advance_ms(1); app_tick_1ms(); }
  run_ms(5);
  TEST_ASSERT_EQUAL_UINT32(1, a.lat[L_BTN_EDGE].n);
  TEST_ASSERT_TRUE(a.lat[L_BTN_EDGE].max_us >= 100000u);
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
  RUN_TEST(test_sd_selftest_is_non_destructive);
  RUN_TEST(test_sd_no_write_if_original_block_unreadable);
  RUN_TEST(test_sd_absent);
  RUN_TEST(test_sd_write_error_reported_and_restore_attempted);
  RUN_TEST(test_pot_and_button_failures);
  RUN_TEST(test_start_checks_and_ready);
  RUN_TEST(test_start_without_imu_fr12);
  RUN_TEST(test_imu_task_200hz);
  RUN_TEST(test_live_servo_follows_tilt);
  RUN_TEST(test_deadzone_from_pot_fr04);
  RUN_TEST(test_angle_override_clamped_fr05);
  RUN_TEST(test_record_playback_roundtrip);
  RUN_TEST(test_recording_survives_restart_fr09);
  RUN_TEST(test_record_gap_counted);
  RUN_TEST(test_sd_absent_blocks_record);
  RUN_TEST(test_telemetry_and_commands_fr11);
  RUN_TEST(test_stats_format_and_latencies);
  RUN_TEST(test_oled_modes_frame_rate_fr10);
  RUN_TEST(test_imu_failure_and_recovery);
  RUN_TEST(test_dump_and_misc_commands);
  RUN_TEST(test_dump_during_play_keeps_all_records);
  RUN_TEST(test_btn_edge_latched_at_debounce);
  return UNITY_END();
}
