/**
 * bus_spi_stm32.c — bus_spi_t для SPI1 + програмний CS#, лише регістри (RM0490, SPI).
 *
 * ЛР3: HAL_SPI_TransmitReceive прибрано. У ЛР2 читання блоку SD тривало 12.2 мс при
 * 6 МГц на шині (19.1 мкс/байт замість 1.33 — analysis.md §8.3): HAL опитував прапорці
 * й стан хендла на кожен байт. Тут — прямий цикл TXE→DR→RXNE→DR, 8-бітний доступ до DR
 * (FRXTH = 1, інакше RXNE чекає 16 біт).
 */
#include "board.h"

typedef struct {
  SPI_TypeDef  *S;
  GPIO_TypeDef *cs_port;
  uint16_t      cs_pin;
} spi_ctx_t;

static spi_ctx_t sd_ctx = { BOARD_SPI, BOARD_SD_CS_Port, BOARD_SD_CS_Pin };

static void sel(void *c, bool on)
{
  spi_ctx_t *x = c;
  x->cs_port->BSRR = on ? (uint32_t)x->cs_pin << 16 : x->cs_pin;   /* CS# активний низький */
}

static int xfer(void *c, const uint8_t *tx, uint8_t *rx, size_t n)
{
  spi_ctx_t *x = c;
  SPI_TypeDef *S = x->S;
  volatile uint8_t *dr = (volatile uint8_t *)&S->DR;
  for (size_t i = 0; i < n; i++) {
    uint32_t guard = 100000u;
    while (!(S->SR & SPI_SR_TXE)) if (!--guard) return ST_ETIMEOUT;
    *dr = tx ? tx[i] : 0xFFu;
    guard = 100000u;
    while (!(S->SR & SPI_SR_RXNE)) if (!--guard) return ST_ETIMEOUT;
    uint8_t b = *dr;
    if (rx) rx[i] = b;
  }
  return ST_OK;
}

static void set_fast(void *c, bool fast)
{
  spi_ctx_t *x = c;
  SPI_TypeDef *S = x->S;
  while (S->SR & SPI_SR_BSY) { }
  S->CR1 &= ~SPI_CR1_SPE;
  MODIFY_REG(S->CR1, SPI_CR1_BR, fast ? BOARD_SPI_BR_FAST : BOARD_SPI_BR_SLOW);
  S->CR1 |= SPI_CR1_SPE;
}

const bus_spi_t board_sd_spi = { &sd_ctx, sel, xfer, set_fast };
