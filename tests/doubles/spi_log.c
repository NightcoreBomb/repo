#include <string.h>
#include "spi_log.h"

spi_event_t spi_log[SPI_LOG_MAX];
int spi_log_n;
bool spi_cs_low;
bool spi_fast;
static const spi_device_t *dev;

static void add(ev_t e, uint8_t mo, uint8_t mi)
{
  if (spi_log_n < SPI_LOG_MAX) spi_log[spi_log_n++] = (spi_event_t){ e, mo, mi };
}

void spi_log_reset(const spi_device_t *d)
{
  spi_log_n = 0; spi_cs_low = false; spi_fast = false; dev = d;
}

static void sel(void *c, bool on)
{
  (void)c;
  if (on == spi_cs_low) return;            /* повторний select без зміни рівня — не подія */
  spi_cs_low = on;
  add(on ? EV_CS_LOW : EV_CS_HIGH, 0, 0);
  if (dev && dev->cs) dev->cs(on);
}

static int xfer(void *c, const uint8_t *tx, uint8_t *rx, size_t n)
{
  (void)c;
  for (size_t i = 0; i < n; i++) {
    uint8_t mo = tx ? tx[i] : 0xFF;
    uint8_t mi = (spi_cs_low && dev && dev->byte) ? dev->byte(mo) : 0xFF;
    if (rx) rx[i] = mi;
    add(EV_BYTE, mo, mi);
  }
  return ST_OK;
}

static void set_fast(void *c, bool f) { (void)c; spi_fast = f; add(f ? EV_FAST : EV_SLOW, 0, 0); }

const bus_spi_t spi_log_bus = { NULL, sel, xfer, set_fast };

int spi_frame(int k, uint8_t *mosi, uint8_t *miso, int max)
{
  int fr = -1, len = 0;
  for (int i = 0; i < spi_log_n; i++) {
    if (spi_log[i].ev == EV_CS_LOW) { fr++; len = 0; continue; }
    if (fr == k && spi_log[i].ev == EV_CS_HIGH) return len;
    if (fr == k && spi_log[i].ev == EV_BYTE && len < max) {
      if (mosi) mosi[len] = spi_log[i].mosi;
      if (miso) miso[len] = spi_log[i].miso;
      len++;
    }
  }
  return fr >= k ? len : -1;
}

int spi_frame_count(void)
{
  int n = 0;
  for (int i = 0; i < spi_log_n; i++) n += spi_log[i].ev == EV_CS_LOW;
  return n;
}

/* перший байт кадру, що не 0xFF (0xFF — опитування готовності перед командою) */
int spi_find_frame(uint8_t first, int from)
{
  static uint8_t b[700];
  for (int k = from; k < spi_frame_count(); k++) {
    int n = spi_frame(k, b, NULL, sizeof b);
    for (int i = 0; i < n; i++) {
      if (b[i] == 0xFF) continue;
      if (b[i] == first) return k;
      break;
    }
  }
  return -1;
}
