/**
 * sdcard.h — microSD у режимі SPI (блоки по 512 байтів).
 * Джерело: SD Physical Layer Simplified Spec v9.10, розд. 7 «SPI Mode»
 * (7.2.1 Mode selection and initialization, 7.3.1 Command format,
 *  7.3.2 Responses, 7.3.3 Control tokens).
 * Залежить лише від Interfaces/bus_spi.h.
 */
#ifndef DEV_SDCARD_H
#define DEV_SDCARD_H

#include <stdbool.h>
#include <stdint.h>
#include "bus_spi.h"

#define SD_BLOCK         512u
#define SD_R1_IDLE       0x01u
#define SD_R1_ILLEGAL    0x04u
#define SD_TOKEN_START   0xFEu
#define SD_DRESP_MASK    0x1Fu
#define SD_DRESP_OK      0x05u

typedef enum { SD_NONE = 0, SD_V1_SDSC, SD_V2_SDSC, SD_V2_SDHC } sd_type_t;

typedef struct {
  const bus_spi_t *bus;
  sd_type_t type;
  uint8_t   last_r1;       /* для діагностики */
  uint32_t  ocr;
  uint16_t  acmd41_tries;  /* скільки разів довелося повторити ACMD41 */
} sdcard_t;

void        sd_setup(sdcard_t *d, const bus_spi_t *bus);
int         sd_init(sdcard_t *d);
int         sd_read_block(sdcard_t *d, uint32_t blk, uint8_t *buf);
int         sd_write_block(sdcard_t *d, uint32_t blk, const uint8_t *buf);
const char *sd_type_str(sd_type_t t);

/* CRC7 командного кадру (poly x^7 + x^3 + 1), повертає 7 біт */
uint8_t     sd_crc7(const uint8_t *p, unsigned n);
/* Сформувати 6-байтовий командний кадр: 01 cmd[5:0] | arg[31:0] | crc7 1 */
void        sd_build_cmd(uint8_t out[6], uint8_t cmd, uint32_t arg);

#endif
