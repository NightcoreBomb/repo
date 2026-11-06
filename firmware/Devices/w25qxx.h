/**
 * w25qxx.h — SPI NOR flash Winbond W25Q64 (для Форми A).
 * У Wokwi цього чипа немає, тож у прошивку для симулятора драйвер не
 * лінкується (див. Makefile, DEVICES_SIM), але він входить у хостові тести:
 * саме тут перевіряється «запис лише після WRITE ENABLE» (методичка, п. 4.6).
 * Джерело: Winbond W25Q64JV datasheet rev. J — §8.1 (instruction set),
 * §8.2.1 Write Enable 06h, §8.2.13 Page Program 02h, §8.2.15 Sector Erase 20h,
 * §8.2.29 JEDEC ID 9Fh, §7.1 Status Register (BUSY = S0, WEL = S1).
 */
#ifndef DEV_W25QXX_H
#define DEV_W25QXX_H

#include <stdint.h>
#include <stddef.h>
#include "bus_spi.h"

#define W25Q_CMD_WREN      0x06u
#define W25Q_CMD_RDSR1     0x05u
#define W25Q_CMD_READ      0x03u
#define W25Q_CMD_PP        0x02u
#define W25Q_CMD_SE4K      0x20u
#define W25Q_CMD_JEDEC     0x9Fu
#define W25Q_SR_BUSY       0x01u
#define W25Q_SR_WEL        0x02u
#define W25Q_PAGE          256u
#define W25Q64_JEDEC_ID    0xEF4017u   /* Winbond, Serial NOR, 64 Мбіт */

typedef struct {
  const bus_spi_t *bus;
  uint32_t jedec;
  uint32_t busy_poll_max;
} w25q_t;

void w25q_setup(w25q_t *d, const bus_spi_t *bus);
int  w25q_identify(w25q_t *d);                         /* ST_EID якщо не W25Q64 */
int  w25q_read(w25q_t *d, uint32_t addr, uint8_t *dst, size_t n);
/* n ≤ 256 і не перетинає межу сторінки (інакше ST_EPARAM) */
int  w25q_page_program(w25q_t *d, uint32_t addr, const uint8_t *src, size_t n);
int  w25q_sector_erase(w25q_t *d, uint32_t addr);
int  w25q_read_status(w25q_t *d, uint8_t *sr);

#endif
