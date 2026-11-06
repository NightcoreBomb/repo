/**
 * ssd1306.h — мінімальний драйвер OLED SSD1306 128×64 по I²C, без кадрового
 * буфера (12 КБ RAM на C031): текст пишеться прямо в GDDRAM по сторінках.
 * Джерело: SSD1306 Advance Information rev 1.1 — §8.1.5 (I²C), §10 (команди).
 */
#ifndef DEV_SSD1306_H
#define DEV_SSD1306_H

#include <stdint.h>
#include "bus_i2c.h"

#define SSD1306_ADDR_SA0_LOW  0x3Cu
#define SSD1306_ADDR_SA0_HIGH 0x3Du
#define SSD1306_CTRL_CMD      0x00u   /* Co=0, D/C#=0 — далі лише команди */
#define SSD1306_CTRL_DATA     0x40u   /* Co=0, D/C#=1 — далі лише дані GDDRAM */
#define SSD1306_WIDTH         128u
#define SSD1306_PAGES         8u
#define SSD1306_CHUNK         16u     /* байтів даних в одній I²C-транзакції */

typedef struct {
  const bus_i2c_t *bus;
  uint8_t addr;
} ssd1306_t;

void ssd1306_setup(ssd1306_t *d, const bus_i2c_t *bus, uint8_t addr);
int  ssd1306_probe(ssd1306_t *d);                 /* ACK на адресу = «ідентифікація» */
int  ssd1306_init(ssd1306_t *d);                  /* послідовність з §10 + display ON */
int  ssd1306_clear(ssd1306_t *d);
/* Рядок шрифтом 5×7 (+1 стовпчик проміжку) у сторінку page (0..7), стовпчик col */
int  ssd1306_text(ssd1306_t *d, uint8_t page, uint8_t col, const char *s);

/* Гліф (5 байтів-стовпчиків, біт0 — верхній піксель); невідомі символи → '?' */
const uint8_t *font5x7_glyph(char c);

#endif
