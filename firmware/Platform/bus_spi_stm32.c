/**
 * bus_spi_stm32.c — реалізація bus_spi_t поверх HAL_SPI + GPIO CS#.
 */
#include <string.h>
#include "board.h"

typedef struct {
  SPI_HandleTypeDef *h;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
} spi_ctx_t;

static spi_ctx_t sd_ctx = { &hspi1, BOARD_SD_CS_Port, BOARD_SD_CS_Pin };

static void sel(void *c, bool on)
{
  spi_ctx_t *x = c;
  HAL_GPIO_WritePin(x->cs_port, x->cs_pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static int xfer(void *c, const uint8_t *tx, uint8_t *rx, size_t n)
{
  spi_ctx_t *x = c;
  static uint8_t ff[32], sink[32];
  memset(ff, 0xFF, sizeof ff);
  while (n) {
    size_t k = n > sizeof ff ? sizeof ff : n;
    HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(x->h, tx ? (uint8_t *)tx : ff,
                                                  rx ? rx : sink, (uint16_t)k, 50);
    if (s != HAL_OK) return s == HAL_TIMEOUT ? ST_ETIMEOUT : (s == HAL_BUSY ? ST_EBUSY : ST_EIO);
    if (tx) tx += k;
    if (rx) rx += k;
    n -= k;
  }
  return ST_OK;
}

static void set_fast(void *c, bool fast)
{
  spi_ctx_t *x = c;
  uint32_t br = fast ? BOARD_SPI_PRESC_FAST : BOARD_SPI_PRESC_SLOW;
  __HAL_SPI_DISABLE(x->h);
  MODIFY_REG(x->h->Instance->CR1, SPI_CR1_BR, br);
  x->h->Init.BaudRatePrescaler = br;
  __HAL_SPI_ENABLE(x->h);
}

const bus_spi_t board_sd_spi = { &sd_ctx, sel, xfer, set_fast };
