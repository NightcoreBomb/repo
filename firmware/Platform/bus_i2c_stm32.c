/**
 * bus_i2c_stm32.c — реалізація bus_i2c_t для I2C1 STM32C0, лише регістри (RM0490, I2C).
 * Єдине місце, де драйвери «зустрічаються» з периферією I²C.
 *
 * ЛР3: HAL_I2C (~2.6 КБ Flash) прибрано — лишився регістровий шлях, який у ЛР2 вже був
 * режимом за замовчуванням (`i2cmode 1`) і пройшов self-test у Wokwi (П-11):
 *   * запис ≥ 2 байтів: CR2 = SADD|NBYTES|AUTOEND|START, перший байт — лише ПІСЛЯ фази
 *     адреси, кожен наступний — по TXIS, після запису — дочекатися скидання TXIS
 *     (емулятор не буферизує TXDR подвійно, «застарілий» TXIS губив байт 1);
 *   * запис 1 байта (вказівник регістра): передзавантаження TXDR до START — як у HAL,
 *     перевірено записом ЛР1 (`S 68W+ 75+ P`), і без паузи на фазу адреси;
 *   * читання: RXNE, з тим самим захистом від «застарілого» прапорця.
 * Пауза до кінця фази адреси в ЛР2 була «≥ 1 мс за HAL_GetTick» (іншого таймера не було) —
 * тепер BOARD_I2C_ADDR_GUARD_US за лічильником тактів (plat_cycles).
 *
 * Будь-який TIMEOUT/BUSY → програмний скид (PE = 0 → 1, RM0490 «I2C software reset»).
 * Інжекція відмови (сценарій ЛР3 6.2.4, plat_i2c_fault(1)): кожна транзакція чекає
 * повний таймаут, як при затиснутій SDA, і повертає TIMEOUT.
 */
#include "board.h"
#include "plat.h"

typedef struct {
  I2C_TypeDef *I;
  uint32_t recoveries, timeouts;
  volatile int fault;
} i2c_ctx_t;

static i2c_ctx_t i2c1_ctx = { BOARD_I2C, 0, 0, 0 };

/* Таймаут транзакції, мс: 20 + ~90 мкс/байт @100 кГц (як у ЛР2) */
static uint32_t to_ms(size_t n) { return 20u + (uint32_t)n; }

static void spin_us(uint32_t us)
{
  uint32_t t0 = plat_cycles(), need = us * plat_cyc_per_us();
  while ((uint32_t)(plat_cycles() - t0) < need) { }
}

static int wait_clear_us(volatile uint32_t *reg, uint32_t mask, uint32_t us)
{
  uint32_t t0 = plat_cycles(), need = us * plat_cyc_per_us();
  while (*reg & mask) if ((uint32_t)(plat_cycles() - t0) >= need) return 0;
  return 1;
}

static void recover(i2c_ctx_t *x)
{
  CLEAR_BIT(x->I->CR1, I2C_CR1_PE);
  for (volatile int i = 0; i < 16; i++) { (void)x->I->CR1; }   /* ≥ 3 такти APB */
  SET_BIT(x->I->CR1, I2C_CR1_PE);
  x->recoveries++;
}

static int fail_timeout(i2c_ctx_t *x) { x->timeouts++; recover(x); return ST_ETIMEOUT; }

/* «SDA затиснута»: BUSY не знімається до кінця таймауту */
static int fault_stuck(i2c_ctx_t *x, size_t n)
{
  uint32_t t0 = HAL_GetTick();
  while (HAL_GetTick() - t0 <= to_ms(n)) { }
  return fail_timeout(x);
}

static void start(I2C_TypeDef *I, uint8_t a, size_t n, uint32_t rd)
{
  MODIFY_REG(I->CR2,
             I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND |
             I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_ADD10,
             ((uint32_t)a << 1) | ((uint32_t)n << I2C_CR2_NBYTES_Pos) |
             I2C_CR2_AUTOEND | rd | I2C_CR2_START);
}

static int wait_idle(i2c_ctx_t *x, uint32_t t0, uint32_t to)
{
  while (x->I->ISR & I2C_ISR_BUSY)
    if (HAL_GetTick() - t0 > to) { recover(x); return ST_EBUSY; }
  x->I->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;
  return ST_OK;
}

/* NACK: AUTOEND сам генерує STOP; якщо STOPF не настає (Wokwi, аномалія А-2) — скид */
static int on_nack(i2c_ctx_t *x)
{
  I2C_TypeDef *I = x->I;
  I->ICR = I2C_ICR_NACKCF;
  uint32_t t1 = HAL_GetTick();
  while (!(I->ISR & I2C_ISR_STOPF)) if (HAL_GetTick() - t1 > 2u) { recover(x); return ST_ENACK; }
  I->ICR = I2C_ICR_STOPCF;
  I->ISR = I2C_ISR_TXE;
  return ST_ENACK;
}

static int finish(i2c_ctx_t *x, uint32_t t0, uint32_t to)
{
  I2C_TypeDef *I = x->I;
  for (;;) {
    uint32_t isr = I->ISR;
    if (isr & I2C_ISR_STOPF) break;
    if (isr & I2C_ISR_NACKF) return on_nack(x);
    if (HAL_GetTick() - t0 > to) return fail_timeout(x);
  }
  I->ICR = I2C_ICR_STOPCF;
  CLEAR_BIT(I->CR2, I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
  return ST_OK;
}

static int w(void *c, uint8_t a, const uint8_t *d, size_t n)
{
  i2c_ctx_t *x = c;
  I2C_TypeDef *I = x->I;
  uint32_t t0 = HAL_GetTick(), to = to_ms(n);
  if (n == 0 || n > 255) return ST_EPARAM;
  if (x->fault) return fault_stuck(x, n);
  int s = wait_idle(x, t0, to);
  if (s != ST_OK) return s;

  I->ISR = I2C_ISR_TXE;                          /* flush TXDR */
  size_t i = 0;
  if (n == 1) {                                  /* вказівник регістра: як HAL, з передзавантаженням */
    I->TXDR = d[0];
    i = 1;
    start(I, a, n, 0);
  } else {
    start(I, a, n, 0);
    spin_us(BOARD_I2C_ADDR_GUARD_US);            /* кінець фази адреси (П-11) */
    if (I->ISR & I2C_ISR_NACKF) return on_nack(x);
  }
  for (; i < n; i++) {
    for (;;) {
      uint32_t isr = I->ISR;
      if (isr & I2C_ISR_TXIS) break;
      if (isr & I2C_ISR_NACKF) return on_nack(x);
      if (isr & (I2C_ISR_BERR | I2C_ISR_ARLO)) { recover(x); return ST_EIO; }
      if (HAL_GetTick() - t0 > to) return fail_timeout(x);
    }
    I->TXDR = d[i];
    if (i + 1 < n) wait_clear_us(&I->ISR, I2C_ISR_TXIS, BOARD_I2C_GUARD_US);
  }
  return finish(x, t0, to);
}

static int r(void *c, uint8_t a, uint8_t *d, size_t n)
{
  i2c_ctx_t *x = c;
  I2C_TypeDef *I = x->I;
  uint32_t t0 = HAL_GetTick(), to = to_ms(n);
  if (n == 0 || n > 255) return ST_EPARAM;
  if (x->fault) return fault_stuck(x, n);
  int s = wait_idle(x, t0, to);
  if (s != ST_OK) return s;
  while (I->ISR & I2C_ISR_RXNE) (void)I->RXDR;  /* злити залишки */
  start(I, a, n, I2C_CR2_RD_WRN);
  for (size_t i = 0; i < n; i++) {
    for (;;) {
      uint32_t isr = I->ISR;
      if (isr & I2C_ISR_RXNE) break;
      if (isr & I2C_ISR_NACKF) return on_nack(x);
      if (HAL_GetTick() - t0 > to) return fail_timeout(x);
    }
    d[i] = (uint8_t)I->RXDR;
    if (i + 1 < n) wait_clear_us(&I->ISR, I2C_ISR_RXNE, BOARD_I2C_GUARD_US);
  }
  return finish(x, t0, to);
}

/* START | a+W | reg | STOP   START | a+R | data… | NACK | STOP
   MPU-6050 зберігає вказівник регістра між транзакціями (запис ЛР1). */
static int wr(void *c, uint8_t a, const uint8_t *wb, size_t nw, uint8_t *rb, size_t nr)
{
  int s = w(c, a, wb, nw);
  return (s == ST_OK) ? r(c, a, rb, nr) : s;
}

/* Лише адреса (NBYTES = 0): ACK → STOP апаратно (AUTOEND) */
static int probe(void *c, uint8_t a)
{
  i2c_ctx_t *x = c;
  I2C_TypeDef *I = x->I;
  uint32_t t0 = HAL_GetTick();
  if (x->fault) return fault_stuck(x, 0);
  int s = wait_idle(x, t0, 5u);
  if (s != ST_OK) return s;
  start(I, a, 0, 0);
  return finish(x, t0, 5u);
}

const bus_i2c_t board_i2c1 = { &i2c1_ctx, w, r, wr, probe };

uint32_t plat_i2c_recoveries(void) { return i2c1_ctx.recoveries; }
uint32_t plat_i2c_timeouts(void)   { return i2c1_ctx.timeouts; }
void     plat_i2c_fault(int m)     { i2c1_ctx.fault = m; }
int      plat_i2c_fault_get(void)  { return i2c1_ctx.fault; }
