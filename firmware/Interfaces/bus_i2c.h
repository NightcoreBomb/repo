/**
 * bus_i2c.h — контракт «I²C-шина» між драйверами пристроїв і платформою.
 *
 * Драйвер (Devices/) отримує вказівник на bus_i2c_t і нічого не знає про
 * HAL_I2C_*: реалізація для STM32 — Platform/bus_i2c_stm32.c, для хостових
 * тестів — tests/doubles/ (mock, fake, stub).
 *
 * Адреса — 7-бітна (0x68), без зсуву; зсув на R/W робить реалізація.
 * Усі функції повертають status_t.
 */
#ifndef IF_BUS_I2C_H
#define IF_BUS_I2C_H

#include <stddef.h>
#include <stdint.h>
#include "status.h"

typedef struct bus_i2c bus_i2c_t;

struct bus_i2c {
  void *ctx;
  /* START, addr+W, n байтів, STOP */
  int (*write)(void *ctx, uint8_t addr7, const uint8_t *data, size_t n);
  /* START, addr+R, n байтів (останній — NACK), STOP */
  int (*read)(void *ctx, uint8_t addr7, uint8_t *data, size_t n);
  /* Запис вказівника регістра + читання. Чи буде між ними repeated START,
     чи STOP+START — вирішує реалізація (див. BOARD_I2C_USE_RESTART). */
  int (*write_read)(void *ctx, uint8_t addr7, const uint8_t *w, size_t nw,
                    uint8_t *r, size_t nr);
  /* Лише адреса: ST_OK, якщо є ACK */
  int (*probe)(void *ctx, uint8_t addr7);
};

/* Зручні обгортки для регістрових пристроїв (8-бітна адреса регістра) */
static inline int i2c_reg_write(const bus_i2c_t *b, uint8_t a, uint8_t reg, uint8_t v)
{
  uint8_t buf[2] = { reg, v };
  return b->write(b->ctx, a, buf, 2);
}

static inline int i2c_reg_read(const bus_i2c_t *b, uint8_t a, uint8_t reg,
                               uint8_t *dst, size_t n)
{
  return b->write_read(b->ctx, a, &reg, 1, dst, n);
}

#endif
