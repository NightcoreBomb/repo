/**
 * ssd1306.c — див. ssd1306.h.
 */
#include <string.h>
#include "ssd1306.h"

void ssd1306_setup(ssd1306_t *d, const bus_i2c_t *bus, uint8_t addr)
{
  d->bus  = bus;
  d->addr = addr;
}

int ssd1306_probe(ssd1306_t *d)
{
  return d->bus->probe(d->bus->ctx, d->addr);
}

static int cmds(ssd1306_t *d, const uint8_t *c, size_t n)
{
  uint8_t buf[1 + 26];
  if (n > sizeof buf - 1) return ST_EPARAM;
  buf[0] = SSD1306_CTRL_CMD;
  memcpy(&buf[1], c, n);
  return d->bus->write(d->bus->ctx, d->addr, buf, n + 1);
}

int ssd1306_init(ssd1306_t *d)
{
  /* §10 + Application note «Software initialization flow» (рис. 2 у AN) */
  static const uint8_t init[] = {
    0xAE,        /* display OFF                                     */
    0xD5, 0x80,  /* clock divide / osc freq (reset value)           */
    0xA8, 0x3F,  /* multiplex ratio = 64                            */
    0xD3, 0x00,  /* display offset = 0                              */
    0x40,        /* start line = 0                                  */
    0x8D, 0x14,  /* charge pump ON (модуль без зовнішнього VCC)     */
    0x20, 0x00,  /* horizontal addressing mode                      */
    0xA1,        /* segment remap (col 127 -> SEG0)                 */
    0xC8,        /* COM scan direction: remapped                    */
    0xDA, 0x12,  /* COM pins: alternative, no L/R remap (128×64)    */
    0x81, 0xCF,  /* contrast                                        */
    0xD9, 0xF1,  /* pre-charge                                      */
    0xDB, 0x40,  /* VCOMH deselect                                  */
    0xA4,        /* RAM -> display                                  */
    0xA6,        /* normal (не інверсний)                           */
    0xAF,        /* display ON                                      */
  };
  return cmds(d, init, sizeof init);
}

static int set_window(ssd1306_t *d, uint8_t page, uint8_t col)
{
  const uint8_t w[] = { 0x21, col, SSD1306_WIDTH - 1, 0x22, page, SSD1306_PAGES - 1 };
  return cmds(d, w, sizeof w);
}

static int data(ssd1306_t *d, const uint8_t *p, size_t n)
{
  uint8_t buf[1 + SSD1306_CHUNK];
  while (n) {
    size_t k = n > SSD1306_CHUNK ? SSD1306_CHUNK : n;
    buf[0] = SSD1306_CTRL_DATA;
    if (p) memcpy(&buf[1], p, k); else memset(&buf[1], 0, k);
    int s = d->bus->write(d->bus->ctx, d->addr, buf, k + 1);
    if (s != ST_OK) return s;
    if (p) p += k;
    n -= k;
  }
  return ST_OK;
}

int ssd1306_clear(ssd1306_t *d)
{
  int s = set_window(d, 0, 0);
  if (s != ST_OK) return s;
  return data(d, NULL, (size_t)SSD1306_WIDTH * SSD1306_PAGES);
}

int ssd1306_text(ssd1306_t *d, uint8_t page, uint8_t col, const char *s)
{
  if (page >= SSD1306_PAGES || col >= SSD1306_WIDTH) return ST_EPARAM;
  int st = set_window(d, page, col);
  if (st != ST_OK) return st;
  uint8_t g[6];
  for (; *s && col + 6u <= SSD1306_WIDTH; s++, col = (uint8_t)(col + 6u)) {
    memcpy(g, font5x7_glyph(*s), 5);
    g[5] = 0x00;
    st = data(d, g, sizeof g);
    if (st != ST_OK) return st;
  }
  return ST_OK;
}
