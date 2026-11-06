/**
 * bus_i2c_stm32.c — реалізація bus_i2c_t поверх HAL_I2C (STM32C0).
 * Єдине місце, де драйвери «зустрічаються» з HAL.
 */
#include "board.h"
#include "plat.h"

typedef struct { I2C_HandleTypeDef *h; volatile int restart; } i2c_ctx_t;
static i2c_ctx_t i2c1_ctx = { &hi2c1, BOARD_I2C_USE_RESTART };

static uint32_t to_ms(size_t n) { return 20u + (uint32_t)n; }   /* ~90 мкс/байт @100 кГц */

static int map(I2C_HandleTypeDef *h, HAL_StatusTypeDef s)
{
  if (s == HAL_OK)      return ST_OK;
  if (s == HAL_BUSY)    return ST_EBUSY;
  if (s == HAL_TIMEOUT) return ST_ETIMEOUT;
  uint32_t e = HAL_I2C_GetError(h);
  if (e & HAL_I2C_ERROR_AF)      return ST_ENACK;     /* Acknowledge Failure */
  if (e & HAL_I2C_ERROR_TIMEOUT) return ST_ETIMEOUT;
  return ST_EIO;
}

static int w(void *c, uint8_t a, const uint8_t *d, size_t n)
{
  i2c_ctx_t *x = c;
  return map(x->h, HAL_I2C_Master_Transmit(x->h, (uint16_t)(a << 1), (uint8_t *)d,
                                           (uint16_t)n, to_ms(n)));
}

static int r(void *c, uint8_t a, uint8_t *d, size_t n)
{
  i2c_ctx_t *x = c;
  return map(x->h, HAL_I2C_Master_Receive(x->h, (uint16_t)(a << 1), d, (uint16_t)n, to_ms(n)));
}

static int wr(void *c, uint8_t a, const uint8_t *wb, size_t nw, uint8_t *rb, size_t nr)
{
  i2c_ctx_t *x = c;
  if (x->restart && nw == 1) {
    /* START | a+W | reg | Sr | a+R | data... | NACK | STOP */
    return map(x->h, HAL_I2C_Mem_Read(x->h, (uint16_t)(a << 1), wb[0], I2C_MEMADD_SIZE_8BIT,
                                      rb, (uint16_t)nr, to_ms(nr + 2)));
  }
  /* START | a+W | reg | STOP   START | a+R | data... | NACK | STOP
     MPU-6050 зберігає вказівник регістра між транзакціями, тож результат
     той самий; саме так WHO_AM_I читався у ЛР1 (docs/captures/lr1_bringup_all.vcd). */
  int s = w(c, a, wb, nw);
  return (s == ST_OK) ? r(c, a, rb, nr) : s;
}

static int probe(void *c, uint8_t a)
{
  i2c_ctx_t *x = c;
  HAL_StatusTypeDef s = HAL_I2C_IsDeviceReady(x->h, (uint16_t)(a << 1), 1, 5);
  if (s == HAL_OK) return ST_OK;
  /* IsDeviceReady не розрізняє NACK і таймаут у ErrorCode — для probe це «немає ACK» */
  return (s == HAL_BUSY) ? ST_EBUSY : ST_ENACK;
}

const bus_i2c_t board_i2c1 = { &i2c1_ctx, w, r, wr, probe };

void plat_i2c_restart(bool on) { i2c1_ctx.restart = on ? 1 : 0; }
