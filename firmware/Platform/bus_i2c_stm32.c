/**
 * bus_i2c_stm32.c — реалізація bus_i2c_t для I2C1 STM32C0.
 * Єдине місце, де драйвери «зустрічаються» з HAL/регістрами I2C.
 *
 * ЗАПИС (журнал проблем, П-11). HAL 1.4.1 у HAL_I2C_Master_Transmit пише перший
 * байт у TXDR ДО генерації START («Preload TX register»), а кожен наступний — по
 * TXIS. У Wokwi після такого передзавантаження TXIS для 2-го байта не настає:
 * однобайтний запис (вказівник 0x75 для WHO_AM_I) працює, двобайтний (`6B 01`)
 * завершується TIMEOUT і лишає периферію з BUSY=1. Тому запис має три режими
 * (перемикаються командою консолі `i2cmode N`, без перезбирання):
 *   0 — HAL_I2C_Master_Transmit (з передзавантаженням) — як у ЛР1;
 *   1 — регістровий, класична послідовність RM0490 «Master transmitter»:
 *       CR2 = SADD|NBYTES|AUTOEND|START, далі кожен байт лише по TXIS — ЗА ЗАМОВЧУВАННЯМ;
 *   2 — HAL_I2C_Mem_Write: перший байт як «адреса регістра» (по TXIS, RELOAD/TCR),
 *       решта — дані.
 * Будь-який TIMEOUT/BUSY → програмний скид периферії (PE=0 → PE=1, RM0490
 * «I2C software reset»), щоб одна невдала транзакція не блокувала шину для
 * інших пристроїв.
 */
#include "board.h"
#include "plat.h"

typedef struct {
  I2C_HandleTypeDef *h;
  volatile int restart;      /* write_read: 1 — repeated START */
  volatile int wmode;        /* режим запису, див. вище */
  uint32_t recoveries;
} i2c_ctx_t;

static i2c_ctx_t i2c1_ctx = { &hi2c1, BOARD_I2C_USE_RESTART, BOARD_I2C_WRITE_MODE, 0 };

static uint32_t to_ms(size_t n) { return 20u + (uint32_t)n; }   /* ~90 мкс/байт @100 кГц */

/* PE=0 скидає автомат I2C і прапорці (BUSY, TXIS, NACKF, STOPF); конфігурація
   TIMINGR/фільтрів зберігається. PE має бути 0 щонайменше 3 такти APB. */
static void recover(i2c_ctx_t *x)
{
  I2C_TypeDef *I = x->h->Instance;
  CLEAR_BIT(I->CR1, I2C_CR1_PE);
  for (volatile int i = 0; i < 16; i++) { (void)I->CR1; }
  SET_BIT(I->CR1, I2C_CR1_PE);
  x->h->State = HAL_I2C_STATE_READY;
  x->h->Mode = HAL_I2C_MODE_NONE;
  x->h->ErrorCode = HAL_I2C_ERROR_NONE;
  __HAL_UNLOCK(x->h);
  x->recoveries++;
}

static int map(i2c_ctx_t *x, HAL_StatusTypeDef s)
{
  if (s == HAL_OK) return ST_OK;
  uint32_t e = HAL_I2C_GetError(x->h);
  int r;
  if (s == HAL_BUSY)                 r = ST_EBUSY;
  else if (s == HAL_TIMEOUT)         r = ST_ETIMEOUT;
  else if (e & HAL_I2C_ERROR_AF)     r = ST_ENACK;     /* Acknowledge Failure */
  else if (e & HAL_I2C_ERROR_TIMEOUT) r = ST_ETIMEOUT;
  else                               r = ST_EIO;
  if (r != ST_ENACK) recover(x);
  return r;
}

/* ---- режим 1: запис по TXIS без передзавантаження ---- */
static int ll_write(i2c_ctx_t *x, uint8_t a, const uint8_t *d, size_t n)
{
  I2C_TypeDef *I = x->h->Instance;
  uint32_t t0 = HAL_GetTick(), to = to_ms(n);
  if (n == 0 || n > 255) return ST_EPARAM;

  while (I->ISR & I2C_ISR_BUSY)
    if (HAL_GetTick() - t0 > to) { recover(x); return ST_EBUSY; }

  I->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;
  I->ISR = I2C_ISR_TXE;                          /* flush TXDR (TXE пишеться 1) */
  MODIFY_REG(I->CR2,
             I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND |
             I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_ADD10,
             ((uint32_t)a << 1) | ((uint32_t)n << I2C_CR2_NBYTES_Pos) |
             I2C_CR2_AUTOEND | I2C_CR2_START);

  for (size_t i = 0; i < n; i++) {
    for (;;) {
      uint32_t isr = I->ISR;
      if (isr & I2C_ISR_TXIS) break;
      if (isr & I2C_ISR_NACKF) goto nack;
      if (isr & (I2C_ISR_BERR | I2C_ISR_ARLO)) { recover(x); return ST_EIO; }
      if (HAL_GetTick() - t0 > to) { recover(x); return ST_ETIMEOUT; }
    }
    I->TXDR = d[i];
  }
  for (;;) {                                     /* AUTOEND → STOP апаратно */
    uint32_t isr = I->ISR;
    if (isr & I2C_ISR_STOPF) break;
    if (isr & I2C_ISR_NACKF) goto nack;
    if (HAL_GetTick() - t0 > to) { recover(x); return ST_ETIMEOUT; }
  }
  I->ICR = I2C_ICR_STOPCF;
  CLEAR_BIT(I->CR2, I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
  return ST_OK;

nack:
  /* адреса або байт без ACK: AUTOEND сам генерує STOP; якщо STOPF не настане
     (так поводиться Wokwi, аналіз А-2) — скидаємо периферію, а не чекаємо */
  I->ICR = I2C_ICR_NACKCF;
  t0 = HAL_GetTick();
  while (!(I->ISR & I2C_ISR_STOPF)) {
    if (HAL_GetTick() - t0 > 2u) { recover(x); return ST_ENACK; }
  }
  I->ICR = I2C_ICR_STOPCF;
  I->ISR = I2C_ISR_TXE;
  CLEAR_BIT(I->CR2, I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
  return ST_ENACK;
}

static int w(void *c, uint8_t a, const uint8_t *d, size_t n)
{
  i2c_ctx_t *x = c;
  if (n >= 2 && x->wmode == 1) return ll_write(x, a, d, n);
  if (n >= 2 && x->wmode == 2)
    return map(x, HAL_I2C_Mem_Write(x->h, (uint16_t)(a << 1), d[0], I2C_MEMADD_SIZE_8BIT,
                                    (uint8_t *)&d[1], (uint16_t)(n - 1), to_ms(n)));
  /* n == 1 (вказівник регістра) — перевірений записом ЛР1 шлях HAL */
  return map(x, HAL_I2C_Master_Transmit(x->h, (uint16_t)(a << 1), (uint8_t *)d,
                                        (uint16_t)n, to_ms(n)));
}

static int r(void *c, uint8_t a, uint8_t *d, size_t n)
{
  i2c_ctx_t *x = c;
  return map(x, HAL_I2C_Master_Receive(x->h, (uint16_t)(a << 1), d, (uint16_t)n, to_ms(n)));
}

static int wr(void *c, uint8_t a, const uint8_t *wb, size_t nw, uint8_t *rb, size_t nr)
{
  i2c_ctx_t *x = c;
  if (x->restart && nw == 1) {
    /* START | a+W | reg | Sr | a+R | data... | NACK | STOP */
    return map(x, HAL_I2C_Mem_Read(x->h, (uint16_t)(a << 1), wb[0], I2C_MEMADD_SIZE_8BIT,
                                   rb, (uint16_t)nr, to_ms(nr + 2)));
  }
  /* START | a+W | reg | STOP   START | a+R | data... | NACK | STOP
     MPU-6050 зберігає вказівник регістра між транзакціями — так WHO_AM_I
     читався у ЛР1 (docs/captures/lr1_bringup_all.vcd). */
  int s = w(c, a, wb, nw);
  return (s == ST_OK) ? r(c, a, rb, nr) : s;
}

static int probe(void *c, uint8_t a)
{
  i2c_ctx_t *x = c;
  HAL_StatusTypeDef s = HAL_I2C_IsDeviceReady(x->h, (uint16_t)(a << 1), 1, 5);
  if (s == HAL_BUSY) {                     /* шину залишила попередня невдала транзакція */
    recover(x);
    s = HAL_I2C_IsDeviceReady(x->h, (uint16_t)(a << 1), 1, 5);
  }
  if (s == HAL_OK) return ST_OK;
  /* IsDeviceReady не розрізняє NACK і таймаут у ErrorCode — для probe це «немає ACK».
     Після NACK у Wokwi STOPF не встановлюється (А-2) — скидаємо, щоб не лишити BUSY. */
  if (x->h->Instance->ISR & I2C_ISR_BUSY) recover(x);
  return (s == HAL_BUSY) ? ST_EBUSY : ST_ENACK;
}

const bus_i2c_t board_i2c1 = { &i2c1_ctx, w, r, wr, probe };

void plat_i2c_restart(bool on)  { i2c1_ctx.restart = on ? 1 : 0; }
void plat_i2c_write_mode(int m) { if (m >= 0 && m <= 2) i2c1_ctx.wmode = m; }
int  plat_i2c_get_write_mode(void) { return i2c1_ctx.wmode; }
uint32_t plat_i2c_recoveries(void) { return i2c1_ctx.recoveries; }
