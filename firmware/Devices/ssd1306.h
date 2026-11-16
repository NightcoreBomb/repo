/**
 * ssd1306.h — мінімальний драйвер OLED SSD1306 128×64 по I²C, без кадрового
 * буфера (12 КБ RAM на C031): текст пишеться прямо в GDDRAM по сторінках.
 * Джерело: SSD1306 Advance Information rev 1.1 — §8.1.5 (I²C), §10 (команди).
 */
#ifndef DEV_SSD1306_H
#define DEV_SSD1306_H

#include <stddef.h>
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

/* ЛР3: рядок цілком — 21 символ × 6 стовпчиків = 126 байтів GDDRAM */
#define SSD1306_LINE_CHARS    21u
#define SSD1306_LINE_BYTES    (SSD1306_LINE_CHARS * 6u)
/* Відрендерити текст у буфер рядка (доповнюючи пробілами) — чиста функція */
void ssd1306_render_line(const char *s, uint8_t out[SSD1306_LINE_BYTES]);
/* Вікно запису: сторінка page, стовпчики col..127 */
int  ssd1306_window(ssd1306_t *d, uint8_t page, uint8_t col);
/* Дані GDDRAM однією транзакцією (n ≤ SSD1306_LINE_BYTES) */
int  ssd1306_write_data(ssd1306_t *d, const uint8_t *p, size_t n);
/* Рядок цілком: вікно + 126 байтів — 2 транзакції замість 1 + 21 у ssd1306_text */
int  ssd1306_line(ssd1306_t *d, uint8_t page, const char *s);

/* Гліф (5 байтів-стовпчиків, біт0 — верхній піксель); невідомі символи → '?' */
const uint8_t *font5x7_glyph(char c);

#endif
