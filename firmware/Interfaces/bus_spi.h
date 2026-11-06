/**
 * bus_spi.h — контракт «SPI-пристрій» (шина + власний CS#).
 *
 * Один bus_spi_t = один пристрій на шині: select() керує саме його CS#.
 * CS# — програмний (GPIO), бо транзакція (команда + адреса + дані) має
 * триматися під одним низьким рівнем CS#, а апаратний NSS на STM32 у режимі
 * master знімається між кадрами (див. звіт, контрольне питання 6).
 */
#ifndef IF_BUS_SPI_H
#define IF_BUS_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "status.h"

typedef struct bus_spi bus_spi_t;

struct bus_spi {
  void *ctx;
  /* true — CS# low (пристрій вибрано), false — CS# high */
  void (*select)(void *ctx, bool on);
  /* повнодуплексний обмін; tx==NULL → шлемо 0xFF; rx==NULL → відкидаємо */
  int  (*xfer)(void *ctx, const uint8_t *tx, uint8_t *rx, size_t n);
  /* перемкнути частоту: slow (≤400 кГц, ініціалізація SD) / fast */
  void (*set_fast)(void *ctx, bool fast);
};

#endif
