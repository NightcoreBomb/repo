/**
 * app.c — ініціалізація пристроїв, головний цикл і консоль UART.
 */
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "selftest.h"
#include "capture.h"
#include "plat.h"
#include "servo.h"
#include "pot.h"

void app_init(app_t *a, const bus_i2c_t *i2c, const bus_spi_t *sd_spi)
{
  memset(a, 0, sizeof *a);
  a->i2c = i2c;
  a->sd_spi = sd_spi;
  mpu6050_setup(&a->imu, i2c, MPU6050_ADDR_AD0_LOW);
  ssd1306_setup(&a->oled, i2c, SSD1306_ADDR_SA0_LOW);
  sd_setup(&a->sd, sd_spi);
  /* FR-12: серво в центр до будь-яких перевірок */
  plat_servo_set_us(0, SERVO_US_CENTER);
  plat_servo_set_us(1, SERVO_US_CENTER);
}

static void cmd_scan(app_t *a)
{
  unsigned n = 0;
  uint32_t t0 = plat_millis();
  plat_log("SCAN");
  for (uint8_t ad = 0x08; ad <= 0x77; ad++)
    if (a->i2c->probe(a->i2c->ctx, ad) == ST_OK) { plat_log(" 0x%02X", ad); n++; }
  /* окремим рядком: час скану показує, скільки HAL чекає на кожну відсутню адресу
     (у записі ЛР1 ~5.9 мс на адресу при 105 мкс на шині — аналіз, аномалія А-2) */
  plat_log(" n=%u\r\nSCAN ms=%lu\r\n", n, (unsigned long)(plat_millis() - t0));
}

static void cmd_imu(app_t *a)
{
  imu_sample_t m;
  imu_measure(a, 1, &m);
  if (m.status != ST_OK) { plat_log("IMU err=%s\r\n", st_str(m.status)); return; }
  plat_log("IMU raw=%d,%d,%d,%d,%d,%d,%d ax=%ld ay=%ld az=%ld an=%lu gx=%ld gy=%ld gz=%ld t_cC=%ld\r\n",
           m.raw.ax, m.raw.ay, m.raw.az, m.raw.temp, m.raw.gx, m.raw.gy, m.raw.gz,
           (long)m.avg.ax_mg, (long)m.avg.ay_mg, (long)m.avg.az_mg, (unsigned long)m.norm_mg,
           (long)m.avg.gx_mdps, (long)m.avg.gy_mdps, (long)m.avg.gz_mdps, (long)m.avg.temp_cC);
}

/* imuchk ax ay az [gx gy gz [t_cC]] — mg, mdps, 0.01 °C */
static void cmd_imuchk(app_t *a, char *args)
{
  int32_t e[7] = { 0, 0, 1000, 0, 0, 0, 2400 };
  static const int32_t tol[3] = { 20, 100, 10 };
  char *p = args, *end;
  for (int i = 0; i < 7; i++) {
    long v = strtol(p, &end, 10);
    if (end == p) break;
    e[i] = (int32_t)v;
    p = end;
  }
  imu_check(a, e, tol);
}

static void help(void)
{
  plat_log("cmds: help selftest stimu imu imuchk scan servo <pan_us> <tilt_us> cap <name> status\r\n");
  capture_list();
}

void app_handle_line(app_t *a, char *line)
{
  char *arg = strchr(line, ' ');
  if (arg) *arg++ = '\0'; else arg = line + strlen(line);

  if (!strcmp(line, "selftest"))      selftest_run(a);
  else if (!strcmp(line, "imu"))      cmd_imu(a);
  else if (!strcmp(line, "imuchk"))   cmd_imuchk(a, arg);
  else if (!strcmp(line, "stimu"))
    plat_log("STIMU %s\r\n", selftest_imu(a) ? "PASS" : "FAIL");   /* вердикт окремим рядком */
  else if (!strcmp(line, "scan"))     cmd_scan(a);
  else if (!strcmp(line, "cap"))      capture_run(a, arg);
  else if (!strcmp(line, "servo")) {
    char *end;
    long p = strtol(arg, &end, 10), t = strtol(end, NULL, 10);
    plat_servo_set_us(0, servo_clamp_us(p));
    plat_servo_set_us(1, servo_clamp_us(t));
    plat_log("SERVO pan_us=%u tilt_us=%u\r\n", plat_servo_get_us(0), plat_servo_get_us(1));
  }
  else if (!strcmp(line, "status"))
    plat_log("STATUS imu=%d oled=%d sd=%d t=%lu\r\n", a->imu_ok, a->oled_ok, a->sd_ok,
             (unsigned long)plat_millis());
  else if (line[0] == '\0') { }
  else if (!strcmp(line, "help"))     help();
  else { plat_log("ERR unknown '%s'\r\n", line); help(); }
}

void app_poll(app_t *a)
{
  static char line[48];
  if (plat_console_getline(line, sizeof line)) app_handle_line(a, line);

  uint32_t p = plat_btn_presses();
  if (p != a->last_presses) {
    a->last_presses = p;
    plat_log("[btn ] pressed, total = %lu\r\n", (unsigned long)p);
  }

  uint32_t now = plat_millis();
  if ((int32_t)(now - a->next_tick) >= 0) {
    a->next_tick = now + APP_TICK_MS;
    a->ticks++;
    plat_led_toggle();
    int raw = plat_pot_read_raw();
    int32_t az = 0;
    if (a->imu_ok) {
      mpu6050_raw_t r;
      if (mpu6050_read_raw(&a->imu, &r) == ST_OK) az = mpu6050_accel_to_mg(r.az, a->imu.accel_fs);
    }
    plat_log("tick %lu | t=%lu ms | az=%ld mg | adc=%d (%lu mV) | btn=%lu\r\n",
             (unsigned long)a->ticks, (unsigned long)now, (long)az, raw,
             (unsigned long)(raw >= 0 ? pot_raw_to_mv((uint32_t)raw) : 0), (unsigned long)p);
  }
}
