/**
 * fake_w25q — FAKE W25Q64: інструкції 06/05/03/02/20/9F, біт WEL, BUSY на
 * кілька читань статусу після запису. Page Program/Erase БЕЗ встановленого
 * WEL ігноруються — як у справжнього чипа (datasheet §8.2.13).
 */
#ifndef FAKE_W25Q_H
#define FAKE_W25Q_H
#include <stdbool.h>
#include <stdint.h>
#include "spi_log.h"

typedef struct {
  uint8_t mem[8192];
  bool wel;
  int  busy_reads;        /* скільки разів RDSR поверне BUSY після операції */
  int  busy_after_op;
  bool wel_stuck_low;     /* модель захищеного від запису чипа (WP#) */
  uint32_t jedec;
  int ignored_programs;   /* PP/SE без WEL */
  /* стан кадру */
  int idx; uint8_t cmd; uint32_t addr;
} fake_w25q_t;

extern fake_w25q_t fake_w25q;
extern const spi_device_t fake_w25q_dev;
void fake_w25q_reset(void);

#endif
