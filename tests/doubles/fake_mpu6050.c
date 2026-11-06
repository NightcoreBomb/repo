#include <string.h>
#include "fake_mpu6050.h"

fake_mpu_t fake_mpu;
int fake_mpu_fail_writes;   /* ≠0 — лише записи ≥2 байтів до MPU повертають цей код (П-11) */

void fake_mpu_set_raw16(uint8_t reg, int16_t v)
{
  fake_mpu.regs[reg]     = (uint8_t)((uint16_t)v >> 8);
  fake_mpu.regs[reg + 1] = (uint8_t)v;
}

void fake_mpu_reset(void)
{
  memset(&fake_mpu, 0, sizeof fake_mpu);
  fake_mpu.addr = 0x68;
  fake_mpu.present = 1;
  fake_mpu.oled_present = 1;
  fake_mpu.regs[0x75] = 0x68;   /* WHO_AM_I */
  fake_mpu.regs[0x6B] = 0x40;   /* SLEEP після POR */
  fake_mpu_set(0, 0, 1000, 0, 0, 0, 2400);   /* як Wokwi за замовчуванням */
}

static int16_t clamp16(int32_t v) { return (int16_t)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v)); }

void fake_mpu_set(int32_t ax, int32_t ay, int32_t az, int32_t gx, int32_t gy, int32_t gz, int32_t t)
{
  /* модель Wokwi: ±2 g → 16384 LSB/g, ±250 °/с → 131 LSB/(°/с), T = raw/340 + 36.53 */
  fake_mpu_set_raw16(0x3B, clamp16(ax * 16384 / 1000));
  fake_mpu_set_raw16(0x3D, clamp16(ay * 16384 / 1000));
  fake_mpu_set_raw16(0x3F, clamp16(az * 16384 / 1000));
  fake_mpu_set_raw16(0x41, clamp16((t - 3653) * 340 / 100));
  fake_mpu_set_raw16(0x43, clamp16(gx * 131 / 1000));
  fake_mpu_set_raw16(0x45, clamp16(gy * 131 / 1000));
  fake_mpu_set_raw16(0x47, clamp16(gz * 131 / 1000));
}

static int sleeping(void) { return (fake_mpu.regs[0x6B] & 0x40) != 0; }

static int target(uint8_t a)
{
  if (a == 0x3C && fake_mpu.oled_present) return 2;
  if (a == fake_mpu.addr && fake_mpu.present) return fake_mpu.force_err ? -1 : 1;
  return 0;
}

static int f_write(void *c, uint8_t a, const uint8_t *d, size_t n)
{
  (void)c;
  int t = target(a);
  if (t == 0) return ST_ENACK;
  if (t < 0) return fake_mpu.force_err;
  if (t == 2) { fake_mpu.oled_writes++; return ST_OK; }
  if (fake_mpu_fail_writes && n >= 2) return fake_mpu_fail_writes;
  fake_mpu.writes++;
  if (n == 0) return ST_OK;
  fake_mpu.ptr = d[0] & 0x7F;
  for (size_t i = 1; i < n; i++) {
    if (fake_mpu.ptr != 0x75) fake_mpu.regs[fake_mpu.ptr] = d[i];  /* WHO_AM_I — RO */
    fake_mpu.ptr = (uint8_t)((fake_mpu.ptr + 1) & 0x7F);
  }
  return ST_OK;
}

static int f_read(void *c, uint8_t a, uint8_t *d, size_t n)
{
  (void)c;
  int t = target(a);
  if (t == 0) return ST_ENACK;
  if (t < 0) return fake_mpu.force_err;
  if (t == 2) return ST_OK;
  fake_mpu.reads++;
  for (size_t i = 0; i < n; i++) {
    uint8_t r = fake_mpu.ptr;
    int data_reg = r >= 0x3B && r <= 0x48;
    d[i] = (data_reg && sleeping()) ? 0x00 : fake_mpu.regs[r];
    fake_mpu.ptr = (uint8_t)((fake_mpu.ptr + 1) & 0x7F);
  }
  return ST_OK;
}

static int f_wr(void *c, uint8_t a, const uint8_t *w, size_t nw, uint8_t *r, size_t nr)
{
  int s = f_write(c, a, w, nw);
  return s == ST_OK ? f_read(c, a, r, nr) : s;
}

static int f_probe(void *c, uint8_t a)
{
  (void)c;
  int t = target(a);
  return t > 0 ? ST_OK : (t < 0 ? fake_mpu.force_err : ST_ENACK);
}

const bus_i2c_t fake_mpu_bus = { NULL, f_write, f_read, f_wr, f_probe };
