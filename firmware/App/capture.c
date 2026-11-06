#include <string.h>
#include "capture.h"
#include "plat.h"
#include "servo.h"
#include "pot.h"

static void hex(const char *tag, const uint8_t *b, unsigned n)
{
  plat_log("%s", tag);
  for (unsigned i = 0; i < n; i++) plat_log(" %02X", b[i]);
  plat_log("\r\n");
}

/* I²C: адресація обох пристроїв, WHO_AM_I, багатобайтове читання 0x3B..0x48 */
static void cap_i2c(app_t *a)
{
  uint8_t who = 0, b[MPU6050_BURST_LEN];
  mpu6050_raw_t r;
  mpu6050_data_t d;
  plat_mark(1);
  int p1 = a->i2c->probe(a->i2c->ctx, a->imu.addr);
  int p2 = a->i2c->probe(a->i2c->ctx, a->oled.addr);
  int s1 = i2c_reg_read(a->i2c, a->imu.addr, MPU6050_REG_WHO_AM_I, &who, 1);
  int s2 = i2c_reg_read(a->i2c, a->imu.addr, MPU6050_REG_ACCEL_XOUT_H, b, sizeof b);
  plat_mark(0);
  plat_log("CAP i2c probe68=%s probe3C=%s who=0x%02X(%s) burst=%s\r\n",
           st_str(p1), st_str(p2), who, st_str(s1), st_str(s2));
  if (s2 == ST_OK) {
    hex("CAP i2c burst3B:", b, sizeof b);
    mpu6050_parse_burst(b, &r);
    mpu6050_convert(&a->imu, &r, &d);
    plat_log("CAP i2c conv ax=%ld ay=%ld az=%ld t_cC=%ld gx=%ld gy=%ld gz=%ld\r\n",
             (long)d.ax_mg, (long)d.ay_mg, (long)d.az_mg, (long)d.temp_cC,
             (long)d.gx_mdps, (long)d.gy_mdps, (long)d.gz_mdps);
  }
}

/* Аномалія, створена свідомо: звернення на 0x69 (AD0=1), якого на шині немає */
static void cap_nack(app_t *a)
{
  uint8_t who = 0;
  plat_mark(1);
  int s = i2c_reg_read(a->i2c, MPU6050_ADDR_AD0_HIGH, MPU6050_REG_WHO_AM_I, &who, 1);
  plat_mark(0);
  plat_log("CAP nack addr=0x69 result=%s (expected NACK)\r\n", st_str(s));
}

/* Repeated START замість STOP+START між записом вказівника і читанням */
static void cap_rs(app_t *a)
{
  uint8_t who = 0, b[MPU6050_BURST_LEN];
  plat_i2c_restart(true);
  plat_mark(1);
  int s1 = i2c_reg_read(a->i2c, a->imu.addr, MPU6050_REG_WHO_AM_I, &who, 1);
  int s2 = i2c_reg_read(a->i2c, a->imu.addr, MPU6050_REG_ACCEL_XOUT_H, b, sizeof b);
  plat_mark(0);
  plat_i2c_restart(false);
  plat_log("CAP rs who=0x%02X(%s) burst=%s\r\n", who, st_str(s1), st_str(s2));
  if (s2 == ST_OK) hex("CAP rs burst3B:", b, sizeof b);
}

/* SPI: повна ініціалізація SD (CMD0/CMD8/CMD55+ACMD41/CMD58) + читання блоку 0 */
static uint8_t blk[SD_BLOCK];
static void cap_spi(app_t *a)
{
  plat_mark(1);
  int s1 = sd_init(&a->sd);
  int s2 = (s1 == ST_OK) ? sd_read_block(&a->sd, 0, blk) : s1;
  plat_mark(0);
  plat_log("CAP spi init=%s type=%s ocr=0x%08lX read0=%s sig=%02X%02X\r\n",
           st_str(s1), sd_type_str(a->sd.type), (unsigned long)a->sd.ocr, st_str(s2),
           blk[510], blk[511]);
}

/* SPI запис з «попередньою командою»: CMD24 + data token + дані + data response.
   (Для W25Q64 на Формі A аналог — WREN 06h перед PP 02h, див. tests/test_w25qxx.c) */
static void cap_spiw(app_t *a)
{
  if (a->sd.type == SD_NONE && sd_init(&a->sd) != ST_OK) {
    plat_log("CAP spiw init FAIL\r\n");
    return;
  }
  int s = sd_read_block(&a->sd, 1, blk);
  plat_mark(1);
  if (s == ST_OK) s = sd_write_block(&a->sd, 1, blk);   /* перезапис тим самим */
  plat_mark(0);
  plat_log("CAP spiw blk=1 write=%s\r\n", st_str(s));
}

/* PWM: дві різні ширини імпульсу, по 5 періодів кожна */
static void cap_pwm(void)
{
  plat_mark(1);
  plat_servo_set_us(1, 1250);
  plat_servo_set_us(0, 1000);
  plat_delay_ms(100);
  plat_servo_set_us(0, 2000);
  plat_delay_ms(100);
  plat_mark(0);
  plat_servo_set_us(0, SERVO_US_CENTER);
  plat_servo_set_us(1, SERVO_US_CENTER);
  plat_log("CAP pwm pan=1000us->2000us tilt=1250us period=%uus\r\n", SERVO_PERIOD_US);
}

/* ADC: цифрового трафіку немає — MARK=1 рівно на час одного перетворення, 20 разів кожні 5 мс */
static void cap_adc(void)
{
  int last = -1;
  uint32_t t0 = plat_millis();
  for (int i = 0; i < 20; i++) {
    while ((int32_t)(plat_millis() - (t0 + (uint32_t)i * 5u)) < 0) { }
    plat_mark(1);
    last = plat_pot_read_raw();
    plat_mark(0);
  }
  plat_log("CAP adc n=20 period_ms=5 last_raw=%d mv=%lu\r\n", last,
           (unsigned long)(last >= 0 ? pot_raw_to_mv((uint32_t)last) : 0));
}

static void cap_uart(void)
{
  plat_mark(1);
  plat_log("CAP uart U=0x55 pattern: UUUU 0123\r\n");
  plat_mark(0);
}

void capture_list(void)
{
  plat_log("cap: i2c nack rs spi spiw pwm adc uart all\r\n");
}

int capture_run(app_t *a, const char *n)
{
  int all = !strcmp(n, "all");
  int known = 0;
  plat_log("CAP %s begin\r\n", n);
  if (all || !strcmp(n, "i2c"))  { cap_i2c(a);  known = 1; plat_delay_ms(5); }
  if (all || !strcmp(n, "nack")) { cap_nack(a); known = 1; plat_delay_ms(5); }
  if (           !strcmp(n, "rs"))   { cap_rs(a);   known = 1; }
  if (all || !strcmp(n, "spi"))  { cap_spi(a);  known = 1; plat_delay_ms(5); }
  if (           !strcmp(n, "spiw")) { cap_spiw(a); known = 1; }
  if (all || !strcmp(n, "pwm"))  { cap_pwm();   known = 1; }
  if (all || !strcmp(n, "adc"))  { cap_adc();   known = 1; }
  if (all || !strcmp(n, "uart")) { cap_uart();  known = 1; }
  if (!known) { plat_log("CAP %s unknown\r\n", n); capture_list(); return 0; }
  plat_log("CAP %s end\r\n", n);
  return 1;
}
