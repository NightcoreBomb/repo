/**
 * w25qxx.c — див. w25qxx.h. Кожна команда — окрема транзакція під утриманим CS#.
 */
#include "w25qxx.h"

void w25q_setup(w25q_t *d, const bus_spi_t *bus)
{
  d->bus = bus;
  d->jedec = 0;
  d->busy_poll_max = 100000;
}

static int cmd_only(w25q_t *d, uint8_t cmd)
{
  d->bus->select(d->bus->ctx, true);
  int s = d->bus->xfer(d->bus->ctx, &cmd, NULL, 1);
  d->bus->select(d->bus->ctx, false);
  return s;
}

int w25q_read_status(w25q_t *d, uint8_t *sr)
{
  uint8_t tx[2] = { W25Q_CMD_RDSR1, 0xFF }, rx[2] = { 0, 0 };
  d->bus->select(d->bus->ctx, true);
  int s = d->bus->xfer(d->bus->ctx, tx, rx, 2);
  d->bus->select(d->bus->ctx, false);
  *sr = rx[1];
  return s;
}

static int wait_idle(w25q_t *d)
{
  uint8_t sr = W25Q_SR_BUSY;
  for (uint32_t i = 0; i < d->busy_poll_max; i++) {
    int s = w25q_read_status(d, &sr);
    if (s != ST_OK) return s;
    if (!(sr & W25Q_SR_BUSY)) return ST_OK;
  }
  return ST_ETIMEOUT;
}

/* WREN + перевірка WEL: якщо біт не встановився — далі не йдемо */
static int write_enable(w25q_t *d)
{
  uint8_t sr = 0;
  int s = cmd_only(d, W25Q_CMD_WREN);
  if (s != ST_OK) return s;
  s = w25q_read_status(d, &sr);
  if (s != ST_OK) return s;
  return (sr & W25Q_SR_WEL) ? ST_OK : ST_EVERIFY;
}

int w25q_identify(w25q_t *d)
{
  uint8_t tx[4] = { W25Q_CMD_JEDEC, 0xFF, 0xFF, 0xFF }, rx[4] = { 0 };
  d->bus->select(d->bus->ctx, true);
  int s = d->bus->xfer(d->bus->ctx, tx, rx, 4);
  d->bus->select(d->bus->ctx, false);
  if (s != ST_OK) return s;
  d->jedec = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
  return d->jedec == W25Q64_JEDEC_ID ? ST_OK : ST_EID;
}

static void addr24(uint8_t *p, uint8_t cmd, uint32_t a)
{
  p[0] = cmd; p[1] = (uint8_t)(a >> 16); p[2] = (uint8_t)(a >> 8); p[3] = (uint8_t)a;
}

int w25q_read(w25q_t *d, uint32_t addr, uint8_t *dst, size_t n)
{
  uint8_t h[4];
  addr24(h, W25Q_CMD_READ, addr);
  d->bus->select(d->bus->ctx, true);
  int s = d->bus->xfer(d->bus->ctx, h, NULL, 4);
  if (s == ST_OK) s = d->bus->xfer(d->bus->ctx, NULL, dst, n);
  d->bus->select(d->bus->ctx, false);
  return s;
}

int w25q_page_program(w25q_t *d, uint32_t addr, const uint8_t *src, size_t n)
{
  if (n == 0 || n > W25Q_PAGE || (addr % W25Q_PAGE) + n > W25Q_PAGE) return ST_EPARAM;
  int s = wait_idle(d);
  if (s == ST_OK) s = write_enable(d);
  if (s != ST_OK) return s;
  uint8_t h[4];
  addr24(h, W25Q_CMD_PP, addr);
  d->bus->select(d->bus->ctx, true);              /* CS# тримаємо: команда+адреса+дані */
  s = d->bus->xfer(d->bus->ctx, h, NULL, 4);
  if (s == ST_OK) s = d->bus->xfer(d->bus->ctx, src, NULL, n);
  d->bus->select(d->bus->ctx, false);             /* фронт CS# запускає програмування */
  if (s != ST_OK) return s;
  return wait_idle(d);
}

int w25q_sector_erase(w25q_t *d, uint32_t addr)
{
  int s = wait_idle(d);
  if (s == ST_OK) s = write_enable(d);
  if (s != ST_OK) return s;
  uint8_t h[4];
  addr24(h, W25Q_CMD_SE4K, addr & ~0xFFFu);
  d->bus->select(d->bus->ctx, true);
  s = d->bus->xfer(d->bus->ctx, h, NULL, 4);
  d->bus->select(d->bus->ctx, false);
  if (s != ST_OK) return s;
  return wait_idle(d);
}
