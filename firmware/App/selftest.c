/**
 * selftest.c — self-test усіх пристроїв PRD у одній прошивці.
 */
#include <string.h>
#include "selftest.h"
#include "plat.h"
#include "servo.h"
#include "pot.h"

static uint8_t sd_orig[SD_BLOCK];
static uint8_t sd_work[SD_BLOCK];

static const char *res(int ok) { return ok ? "PASS" : "FAIL"; }

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

void imu_measure(app_t *a, int n, imu_sample_t *o)
{
  int32_t acc[7] = { 0 };          /* 8 відліків × ≤3.3e5 — без переповнення */
  memset(o, 0, sizeof *o);
  if (n < 1) n = 1;
  for (int i = 0; i < n; i++) {
    mpu6050_data_t d;
    o->status = mpu6050_read_raw(&a->imu, &o->raw);
    if (o->status != ST_OK) return;
    mpu6050_convert(&a->imu, &o->raw, &d);
    acc[0] += d.ax_mg;   acc[1] += d.ay_mg;   acc[2] += d.az_mg;
    acc[3] += d.gx_mdps; acc[4] += d.gy_mdps; acc[5] += d.gz_mdps;
    acc[6] += d.temp_cC;
  }
  o->avg.ax_mg   = (acc[0] / n); o->avg.ay_mg   = (acc[1] / n);
  o->avg.az_mg   = (acc[2] / n); o->avg.gx_mdps = (acc[3] / n);
  o->avg.gy_mdps = (acc[4] / n); o->avg.gz_mdps = (acc[5] / n);
  o->avg.temp_cC = (acc[6] / n);
  o->norm_mg = mpu6050_accel_norm_mg(&o->avg);
}

static int raw_is_stuck(const mpu6050_raw_t *r)
{
  /* усі 0x0000 або усі 0xFFFF — типова картина «SDA висить» або сплячого чипа */
  const int16_t *p = &r->ax;
  int zeros = 0, ones = 0;
  for (int i = 0; i < 7; i++) { zeros += (p[i] == 0); ones += (p[i] == -1); }
  return zeros == 7 || ones == 7;
}

int selftest_imu(app_t *a)
{
  imu_sample_t m;
  int s = mpu6050_identify(&a->imu);
  if (s != ST_OK) {
    plat_log("ST imu FAIL step=identify err=%s who=0x%02X addr=0x%02X\r\n",
             st_str(s), a->imu.who_am_i, a->imu.addr);
    return a->imu_ok = 0;
  }
  s = mpu6050_init(&a->imu);
  if (s != ST_OK) {
    plat_log("ST imu FAIL step=init err=%s reg=0x%02X i2cmode=%d\r\n", st_str(s),
             a->imu.fail_reg, plat_i2c_get_write_mode());
    plat_i2c_diag();
    return a->imu_ok = 0;
  }
  plat_delay_ms(20);                       /* PLL і перший відлік після пробудження */
  imu_measure(a, ST_IMU_SAMPLES, &m);
  if (m.status != ST_OK) {
    plat_log("ST imu FAIL step=read err=%s\r\n", st_str(m.status));
    return a->imu_ok = 0;
  }
  int32_t gmax = iabs32(m.avg.gx_mdps);
  if (iabs32(m.avg.gy_mdps) > gmax) gmax = iabs32(m.avg.gy_mdps);
  if (iabs32(m.avg.gz_mdps) > gmax) gmax = iabs32(m.avg.gz_mdps);

  int c_norm = iabs32((int32_t)m.norm_mg - ST_IMU_NORM_MG) <= ST_IMU_NORM_TOL_MG;
  int c_gyro = gmax <= ST_IMU_GYRO_MAX_MDPS;
  int c_temp = m.avg.temp_cC >= ST_IMU_TEMP_MIN_CC && m.avg.temp_cC <= ST_IMU_TEMP_MAX_CC;
  int c_live = !raw_is_stuck(&m.raw);
  int ok = c_norm && c_gyro && c_temp && c_live;

  plat_log("ST imu %s id=0x%02X pwr=0x%02X ax=%ld ay=%ld az=%ld an=%lu gx=%ld gy=%ld gz=%ld "
           "t_cC=%ld c_norm=%d c_gyro=%d c_temp=%d c_live=%d\r\n",
           res(ok), a->imu.who_am_i, a->imu.pwr_mgmt_1,
           (long)m.avg.ax_mg, (long)m.avg.ay_mg, (long)m.avg.az_mg, (unsigned long)m.norm_mg,
           (long)m.avg.gx_mdps, (long)m.avg.gy_mdps, (long)m.avg.gz_mdps,
           (long)m.avg.temp_cC, c_norm, c_gyro, c_temp, c_live);
  return a->imu_ok = ok;
}

int imu_check(app_t *a, const int32_t e[7], const int32_t tol[3])
{
  imu_sample_t m;
  imu_measure(a, 4, &m);
  if (m.status != ST_OK) {
    plat_log("IMUCHK FAIL\r\nIMUCHK err=%s\r\n", st_str(m.status));
    return 0;
  }
  const int32_t got[7] = { m.avg.ax_mg, m.avg.ay_mg, m.avg.az_mg,
                           m.avg.gx_mdps, m.avg.gy_mdps, m.avg.gz_mdps, m.avg.temp_cC };
  int ok = 1;
  for (int i = 0; i < 7; i++) {
    int32_t t = tol[i < 3 ? 0 : (i < 6 ? 1 : 2)];
    if (iabs32(got[i] - e[i]) > t) ok = 0;
  }
  /* вердикт — окремим рядком без змінних частин: його чекає automation
     scenario Wokwi (tests/sim/mpu6050.test.yaml, крок wait-serial) */
  plat_log("IMUCHK %s\r\n", res(ok));
  plat_log("IMUCHK got ax=%ld ay=%ld az=%ld gx=%ld gy=%ld gz=%ld t_cC=%ld\r\n",
           (long)got[0], (long)got[1], (long)got[2], (long)got[3], (long)got[4],
           (long)got[5], (long)got[6]);
  return ok;
}

int selftest_oled(app_t *a)
{
  int s = ssd1306_probe(&a->oled);
  const char *step = "probe";
  if (s == ST_OK) { step = "init";  s = ssd1306_init(&a->oled); }
  if (s == ST_OK) { step = "clear"; s = ssd1306_clear(&a->oled); }
  if (s == ST_OK) { step = "text";  s = ssd1306_text(&a->oled, 0, 0, "LR2 SELF-TEST"); }
  a->oled_ok = (s == ST_OK);
  if (a->oled_ok) plat_log("ST oled PASS addr=0x%02X\r\n", a->oled.addr);
  else {
    plat_log("ST oled FAIL step=%s err=%s addr=0x%02X\r\n", step, st_str(s), a->oled.addr);
    plat_i2c_diag();
  }
  return a->oled_ok;
}

static uint8_t pattern(unsigned i) { return (uint8_t)(0xA5u ^ (i * 7u)); }

int selftest_sd(app_t *a)
{
  int s = sd_init(&a->sd);
  if (s != ST_OK) {
    plat_log("ST sd FAIL step=init err=%s r1=0x%02X\r\n", st_str(s), a->sd.last_r1);
    return a->sd_ok = 0;
  }
  /* блок 0: на FAT-образі Wokwi — завантажувальний сектор з 0x55AA */
  s = sd_read_block(&a->sd, 0, sd_work);
  if (s != ST_OK) {
    plat_log("ST sd FAIL step=read0 err=%s\r\n", st_str(s));
    return a->sd_ok = 0;
  }
  unsigned sig = ((unsigned)sd_work[510] << 8) | sd_work[511];

  /* неруйнівний RW: зберегти → записати шаблон → прочитати й звірити → відновити */
  const char *step = "rd";
  s = sd_read_block(&a->sd, ST_SD_TEST_BLOCK, sd_orig);
  if (s == ST_OK) {
    step = "wr";
    for (unsigned i = 0; i < SD_BLOCK; i++) sd_work[i] = pattern(i);
    s = sd_write_block(&a->sd, ST_SD_TEST_BLOCK, sd_work);
  }
  if (s == ST_OK) {
    step = "verify";
    memset(sd_work, 0, sizeof sd_work);
    s = sd_read_block(&a->sd, ST_SD_TEST_BLOCK, sd_work);
    for (unsigned i = 0; s == ST_OK && i < SD_BLOCK; i++)
      if (sd_work[i] != pattern(i)) s = ST_EVERIFY;
  }
  /* відновлення — ЛИШЕ якщо оригінал справді прочитано (інакше записали б
     вміст буфера від попереднього виклику; дефект знайдено тестом
     test_sd_no_write_if_original_block_unreadable) */
  if (strcmp(step, "rd") != 0) {
    int r = sd_write_block(&a->sd, ST_SD_TEST_BLOCK, sd_orig);
    if (s == ST_OK) { step = "restore"; s = r; }
  }

  a->sd_ok = (s == ST_OK);
  plat_log("ST sd %s type=%s ocr=0x%08lX acmd41=%u sig=0x%04X rw_blk=%u rw=%s%s%s\r\n",
           res(a->sd_ok), sd_type_str(a->sd.type), (unsigned long)a->sd.ocr,
           a->sd.acmd41_tries, sig, ST_SD_TEST_BLOCK, st_str(s),
           a->sd_ok ? "" : " step=", a->sd_ok ? "" : step);
  return a->sd_ok;
}

int selftest_servo(void)
{
  plat_servo_set_us(0, SERVO_US_CENTER);
  plat_servo_set_us(1, SERVO_US_CENTER);
  uint16_t p = plat_servo_get_us(0), t = plat_servo_get_us(1);
  int ok = p == SERVO_US_CENTER && t == SERVO_US_CENTER &&
           servo_cdeg_to_us(9000) == SERVO_US_CENTER;
  plat_log("ST servo %s pan_us=%u tilt_us=%u period_us=%u\r\n", res(ok), p, t, SERVO_PERIOD_US);
  return ok;
}

int selftest_pot(void)
{
  int raw = plat_pot_read_raw();
  int ok = raw >= 0 && raw <= (int)POT_ADC_MAX;
  plat_log("ST pot %s raw=%d mv=%lu dz_cdeg=%ld\r\n", res(ok), raw,
           (unsigned long)(ok ? pot_raw_to_mv((uint32_t)raw) : 0),
           (long)(ok ? pot_raw_to_deadzone_cdeg((uint32_t)raw) : 0));
  return ok;
}

int selftest_btn(void)
{
  int lvl = plat_btn_level();
  int ok = (lvl == 1);           /* відпущена + працює pull-up */
  plat_log("ST btn %s level=%d presses=%lu edges=%lu debounce_ms=%u\r\n", res(ok), lvl,
           (unsigned long)app_btn_presses(), (unsigned long)plat_btn_edges(), BUTTON_DEBOUNCE_MS);
  return ok;
}

st_summary_t selftest_run(app_t *a)
{
  st_summary_t s = { 0, 0 };
  int r[6];
  plat_log("ST BEGIN fw=%s board=%s sysclk=%lu\r\n", APP_FW_TAG, plat_board_name(),
           (unsigned long)plat_sysclk_hz());
  r[0] = selftest_imu(a);
  r[1] = selftest_oled(a);
  r[2] = selftest_sd(a);
  r[3] = selftest_servo();
  r[4] = selftest_pot();
  r[5] = selftest_btn();
  for (int i = 0; i < 6; i++) { if (r[i]) s.pass++; else s.fail++; }
  plat_log("ST END pass=%d fail=%d\r\n", s.pass, s.fail);
  plat_i2c_diag();

  if (a->oled_ok) {             /* FR-12: статус на OLED */
    ssd1306_text(&a->oled, 2, 0, a->imu_ok ? "IMU  OK" : "IMU NOT FOUND");
    ssd1306_text(&a->oled, 3, 0, a->sd_ok  ? "SD   OK" : "SD  ERROR");
    ssd1306_text(&a->oled, 5, 0, s.fail ? "SELF-TEST FAIL" : "SELF-TEST PASS");
  }
  return s;
}
