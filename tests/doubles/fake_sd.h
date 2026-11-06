/**
 * fake_sd — FAKE microSD у режимі SPI: розбирає командні кадри (з перевіркою
 * CRC7 для CMD0/CMD8, як справжня карта), моделює idle → ready через ACMD41,
 * має 8 блоків пам'яті, підтримує CMD17/CMD24 з токенами даних.
 */
#ifndef FAKE_SD_H
#define FAKE_SD_H
#include <stdbool.h>
#include <stdint.h>
#include "spi_log.h"

typedef struct {
  bool sdhc;            /* CCS у OCR */
  bool v1;              /* CMD8 → illegal command */
  bool absent;          /* MISO завжди 0xFF */
  int  acmd41_busy;     /* скільки ACMD41 відповідатиме 0x01 */
  bool reject_write;    /* data response = 0x0D (write error) */
  int  fail_read_blk;   /* ≥0 — CMD17 на цей блок дає R1 = 0x08 (помилка) */
  uint8_t mem[8][512];
  /* внутрішній стан */
  bool idle, ready, app;
  uint8_t cmd[6]; int ncmd;
  uint8_t q[16]; int qh, qt;
  int  wstate; int widx; uint32_t wblk; uint8_t wbuf[514];
  int  bad_crc_seen;
  int  writes;           /* скільки CMD24 прийнято */
} fake_sd_t;

extern fake_sd_t fake_sd;
extern const spi_device_t fake_sd_dev;
void fake_sd_reset(void);

#endif
