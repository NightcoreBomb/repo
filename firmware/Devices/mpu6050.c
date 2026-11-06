/**
 * mpu6050.c — див. mpu6050.h. Жодного #include HAL: шина — через bus_i2c_t.
 */
#include "mpu6050.h"

void mpu6050_setup(mpu6050_t *d, const bus_i2c_t *bus, uint8_t addr)
{
  d->bus        = bus;
  d->addr       = addr;
  d->accel_fs   = MPU_ACCEL_2G;
  d->gyro_fs    = MPU_GYRO_250;
  d->who_am_i   = 0;
  d->pwr_mgmt_1 = 0xFF;
}

int mpu6050_identify(mpu6050_t *d)
{
  uint8_t v = 0;
  int s = i2c_reg_read(d->bus, d->addr, MPU6050_REG_WHO_AM_I, &v, 1);
  if (s != ST_OK) return s;
  d->who_am_i = v;
  return (v == MPU6050_WHO_AM_I_VAL) ? ST_OK : ST_EID;
}

int mpu6050_init(mpu6050_t *d)
{
  /* Порядок важливий: спершу пробудження (після POR PWR_MGMT_1 = 0x40, SLEEP=1,
     RM §4.28), інакше запис конфігурації в деяких ревізіях ігнорується. */
  static const uint8_t seq[][2] = {
    { MPU6050_REG_PWR_MGMT_1, 0x01 },  /* SLEEP=0, CLKSEL=1 (PLL, X gyro) */
    { MPU6050_REG_CONFIG,     0x03 },  /* DLPF_CFG=3: 44 Гц, Fs_gyro=1 кГц */
    { MPU6050_REG_SMPLRT_DIV, 0x04 },  /* 1000/(1+4) = 200 Гц              */
  };
  int s;
  for (unsigned i = 0; i < sizeof seq / sizeof seq[0]; i++) {
    s = i2c_reg_write(d->bus, d->addr, seq[i][0], seq[i][1]);
    if (s != ST_OK) return s;
  }
  s = i2c_reg_write(d->bus, d->addr, MPU6050_REG_GYRO_CONFIG, (uint8_t)(d->gyro_fs << 3));
  if (s != ST_OK) return s;
  s = i2c_reg_write(d->bus, d->addr, MPU6050_REG_ACCEL_CONFIG, (uint8_t)(d->accel_fs << 3));
  if (s != ST_OK) return s;

  /* Read-back лише для звіту (у симуляторі може не бути реалізований) */
  uint8_t v = 0xFF;
  if (i2c_reg_read(d->bus, d->addr, MPU6050_REG_PWR_MGMT_1, &v, 1) == ST_OK)
    d->pwr_mgmt_1 = v;
  return ST_OK;
}

void mpu6050_parse_burst(const uint8_t b[MPU6050_BURST_LEN], mpu6050_raw_t *r)
{
  /* Регістри *_H йдуть першими: big-endian (RM §4.17–4.19) */
#define BE16(i) ((int16_t)(uint16_t)(((uint16_t)b[(i)] << 8) | b[(i) + 1]))
  r->ax = BE16(0);  r->ay = BE16(2);  r->az = BE16(4);
  r->temp = BE16(6);
  r->gx = BE16(8);  r->gy = BE16(10); r->gz = BE16(12);
#undef BE16
}

int mpu6050_read_raw(mpu6050_t *d, mpu6050_raw_t *raw)
{
  uint8_t b[MPU6050_BURST_LEN];
  int s = i2c_reg_read(d->bus, d->addr, MPU6050_REG_ACCEL_XOUT_H, b, sizeof b);
  if (s != ST_OK) return s;
  mpu6050_parse_burst(b, raw);
  return ST_OK;
}

int32_t mpu6050_accel_lsb_per_g(mpu_accel_fs_t fs)
{
  /* PS §6.2: 16384 / 8192 / 4096 / 2048 LSB/g */
  return 16384 >> (unsigned)fs;
}

int32_t mpu6050_gyro_lsb_per_dps_x10(mpu_gyro_fs_t fs)
{
  /* PS §6.1: 131 / 65.5 / 32.8 / 16.4 LSB/(°/s) — зберігаємо ×10 */
  static const int32_t t[4] = { 1310, 655, 328, 164 };
  return t[(unsigned)fs & 3u];
}

/* Ділення з округленням до найближчого (для від'ємних — симетрично).
   32-бітна арифметика достатня: |raw|·10000 ≤ 3.3e8 < 2^31, а 64-бітне
   ділення на M0+ коштує ~0.6 КБ Flash (__divdi3). */
static int32_t div_round(int32_t num, int32_t den)
{
  return (num >= 0) ? (num + den / 2) / den : (num - den / 2) / den;
}

int32_t mpu6050_accel_to_mg(int16_t raw, mpu_accel_fs_t fs)
{
  return div_round((int32_t)raw * 1000, mpu6050_accel_lsb_per_g(fs));
}

int32_t mpu6050_gyro_to_mdps(int16_t raw, mpu_gyro_fs_t fs)
{
  return div_round((int32_t)raw * 10000, mpu6050_gyro_lsb_per_dps_x10(fs));
}

int32_t mpu6050_temp_to_cC(int16_t raw)
{
  /* T[°C] = raw/340 + 36.53  →  T[0.01 °C] = raw*100/340 + 3653 */
  return div_round((int32_t)raw * 100, 340) + 3653;
}

void mpu6050_convert(const mpu6050_t *d, const mpu6050_raw_t *r, mpu6050_data_t *o)
{
  o->ax_mg   = mpu6050_accel_to_mg(r->ax, d->accel_fs);
  o->ay_mg   = mpu6050_accel_to_mg(r->ay, d->accel_fs);
  o->az_mg   = mpu6050_accel_to_mg(r->az, d->accel_fs);
  o->gx_mdps = mpu6050_gyro_to_mdps(r->gx, d->gyro_fs);
  o->gy_mdps = mpu6050_gyro_to_mdps(r->gy, d->gyro_fs);
  o->gz_mdps = mpu6050_gyro_to_mdps(r->gz, d->gyro_fs);
  o->temp_cC = mpu6050_temp_to_cC(r->temp);
}

uint32_t isqrt_u32(uint32_t x)
{
  uint32_t res = 0, bit = (uint32_t)1 << 30;
  while (bit > x) bit >>= 2;
  while (bit) {
    if (x >= res + bit) { x -= res + bit; res = (res >> 1) + bit; }
    else                { res >>= 1; }
    bit >>= 2;
  }
  return (uint32_t)res;
}

uint32_t mpu6050_accel_norm_mg(const mpu6050_data_t *v)
{
  /* |a| ≤ 16 g = 16000 mg на вісь → сума квадратів ≤ 7.7e8 < 2^32 */
  uint32_t x = (uint32_t)(v->ax_mg < 0 ? -v->ax_mg : v->ax_mg);
  uint32_t y = (uint32_t)(v->ay_mg < 0 ? -v->ay_mg : v->ay_mg);
  uint32_t z = (uint32_t)(v->az_mg < 0 ? -v->az_mg : v->az_mg);
  return isqrt_u32(x * x + y * y + z * z);
}
