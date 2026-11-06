/**
 * sdcard.c — див. sdcard.h. Таймаути — у кількості опитаних байтів/спроб,
 * а не в мілісекундах: драйвер не залежить від таймера платформи, а на 375 кГц
 * 1 байт ≈ 21 мкс, тож порядок величин відомий (див. docs/pinout.md §Шини).
 */
#include "sdcard.h"

#define CMD0    0u   /* GO_IDLE_STATE        */
#define CMD8    8u   /* SEND_IF_COND         */
#define CMD16  16u   /* SET_BLOCKLEN         */
#define CMD17  17u   /* READ_SINGLE_BLOCK    */
#define CMD24  24u   /* WRITE_BLOCK          */
#define CMD55  55u   /* APP_CMD (префікс ACMD) */
#define CMD58  58u   /* READ_OCR             */
#define ACMD41 41u   /* SD_SEND_OP_COND      */

#define R1_POLL_BYTES      10u
#define READY_POLL_BYTES   5000u
#define TOKEN_POLL_BYTES   20000u
#define BUSY_POLL_BYTES    60000u
#define ACMD41_MAX_TRIES   2000u

static uint8_t xchg(sdcard_t *d, uint8_t b)
{
  uint8_t r = 0xFF;
  d->bus->xfer(d->bus->ctx, &b, &r, 1);
  return r;
}

uint8_t sd_crc7(const uint8_t *p, unsigned n)
{
  uint8_t crc = 0;
  for (unsigned i = 0; i < n; i++) {
    uint8_t b = p[i];
    for (unsigned k = 0; k < 8; k++) {
      crc <<= 1;
      if ((b ^ crc) & 0x80) crc ^= 0x09;
      b <<= 1;
    }
  }
  return crc & 0x7F;
}

void sd_build_cmd(uint8_t f[6], uint8_t cmd, uint32_t arg)
{
  f[0] = (uint8_t)(0x40u | (cmd & 0x3Fu));
  f[1] = (uint8_t)(arg >> 24);
  f[2] = (uint8_t)(arg >> 16);
  f[3] = (uint8_t)(arg >> 8);
  f[4] = (uint8_t)arg;
  f[5] = (uint8_t)((sd_crc7(f, 5) << 1) | 1u);
}

static void deselect(sdcard_t *d)
{
  d->bus->select(d->bus->ctx, false);
  xchg(d, 0xFF);                 /* 8 тактів після CS↑ — карта звільняє DO */
}

static int wait_ready(sdcard_t *d)
{
  for (unsigned i = 0; i < READY_POLL_BYTES; i++)
    if (xchg(d, 0xFF) == 0xFF) return ST_OK;
  return ST_ETIMEOUT;
}

/* Надіслати команду під CS# і повернути R1 (CS# лишається низьким) */
static int send_cmd(sdcard_t *d, uint8_t cmd, uint32_t arg, uint8_t *r1)
{
  uint8_t f[6];
  d->bus->select(d->bus->ctx, true);
  if (cmd != CMD0 && wait_ready(d) != ST_OK) return ST_ETIMEOUT;
  sd_build_cmd(f, cmd, arg);
  int s = d->bus->xfer(d->bus->ctx, f, NULL, sizeof f);
  if (s != ST_OK) return s;
  for (unsigned i = 0; i < R1_POLL_BYTES; i++) {   /* NCR = 0..8 байтів */
    uint8_t r = xchg(d, 0xFF);
    if (!(r & 0x80)) { *r1 = r; d->last_r1 = r; return ST_OK; }
  }
  return ST_ENORESP;
}

void sd_setup(sdcard_t *d, const bus_spi_t *bus)
{
  d->bus = bus;
  d->type = SD_NONE;
  d->last_r1 = 0xFF;
  d->ocr = 0;
  d->acmd41_tries = 0;
}

int sd_init(sdcard_t *d)
{
  uint8_t r1 = 0xFF, r[4];
  int s;
  d->type = SD_NONE;
  d->bus->set_fast(d->bus->ctx, false);

  /* ≥74 тактів при CS# = high (§6.4.1) — шлемо 80 */
  d->bus->select(d->bus->ctx, false);
  for (int i = 0; i < 10; i++) xchg(d, 0xFF);

  /* CMD0 → R1 = 0x01 (idle). CRC обов'язковий: 0x95 */
  for (int i = 0; ; i++) {
    s = send_cmd(d, CMD0, 0, &r1);
    deselect(d);
    if (s == ST_OK && r1 == SD_R1_IDLE) break;
    if (i >= 9) return (s != ST_OK) ? s : ST_ENORESP;
  }

  /* CMD8 (VHS=1: 2.7–3.6 В, check pattern 0xAA). CRC обов'язковий: 0x87 */
  bool v2 = false;
  s = send_cmd(d, CMD8, 0x000001AAu, &r1);
  if (s != ST_OK) { deselect(d); return s; }
  if (!(r1 & SD_R1_ILLEGAL)) {
    d->bus->xfer(d->bus->ctx, NULL, r, 4);          /* R7 */
    if ((r[2] & 0x0F) != 0x01 || r[3] != 0xAA) { deselect(d); return ST_EVERIFY; }
    v2 = true;
  }
  deselect(d);

  /* ACMD41 = CMD55 + CMD41, HCS=1 для v2 — до R1 = 0x00 */
  for (d->acmd41_tries = 1; ; d->acmd41_tries++) {
    s = send_cmd(d, CMD55, 0, &r1);
    deselect(d);
    if (s != ST_OK) return s;
    s = send_cmd(d, ACMD41, v2 ? 0x40000000u : 0, &r1);
    deselect(d);
    if (s != ST_OK) return s;
    if (r1 == 0x00) break;
    if (r1 & ~SD_R1_IDLE) return ST_ENORESP;
    if (d->acmd41_tries >= ACMD41_MAX_TRIES) return ST_ETIMEOUT;
  }

  if (v2) {
    s = send_cmd(d, CMD58, 0, &r1);
    if (s == ST_OK) {
      d->bus->xfer(d->bus->ctx, NULL, r, 4);
      d->ocr = ((uint32_t)r[0] << 24) | ((uint32_t)r[1] << 16) | ((uint32_t)r[2] << 8) | r[3];
    }
    deselect(d);
    if (s != ST_OK || r1 != 0) return s != ST_OK ? s : ST_ENORESP;
    d->type = (d->ocr & 0x40000000u) ? SD_V2_SDHC : SD_V2_SDSC;   /* CCS */
  } else {
    d->type = SD_V1_SDSC;
  }
  if (d->type != SD_V2_SDHC) {                       /* SDSC: блок = 512 явно */
    s = send_cmd(d, CMD16, SD_BLOCK, &r1);
    deselect(d);
    if (s != ST_OK || r1 != 0) { d->type = SD_NONE; return s != ST_OK ? s : ST_ENORESP; }
  }
  d->bus->set_fast(d->bus->ctx, true);
  return ST_OK;
}

static uint32_t blk_addr(const sdcard_t *d, uint32_t blk)
{
  return (d->type == SD_V2_SDHC) ? blk : blk * SD_BLOCK;   /* SDSC — байтова адреса */
}

int sd_read_block(sdcard_t *d, uint32_t blk, uint8_t *buf)
{
  uint8_t r1, tok = 0xFF, crc[2];
  if (d->type == SD_NONE) return ST_EPARAM;
  int s = send_cmd(d, CMD17, blk_addr(d, blk), &r1);
  if (s != ST_OK || r1 != 0) { deselect(d); return s != ST_OK ? s : ST_ENORESP; }
  for (unsigned i = 0; i < TOKEN_POLL_BYTES && tok == 0xFF; i++) tok = xchg(d, 0xFF);
  if (tok != SD_TOKEN_START) { deselect(d); return tok == 0xFF ? ST_ETIMEOUT : ST_EIO; }
  s = d->bus->xfer(d->bus->ctx, NULL, buf, SD_BLOCK);
  d->bus->xfer(d->bus->ctx, NULL, crc, 2);           /* CRC16 даних — не перевіряємо */
  deselect(d);
  return s;
}

int sd_write_block(sdcard_t *d, uint32_t blk, const uint8_t *buf)
{
  uint8_t r1, tok[2] = { 0xFF, SD_TOKEN_START }, crc[2] = { 0xFF, 0xFF };
  if (d->type == SD_NONE) return ST_EPARAM;
  int s = send_cmd(d, CMD24, blk_addr(d, blk), &r1);
  if (s != ST_OK || r1 != 0) { deselect(d); return s != ST_OK ? s : ST_ENORESP; }
  d->bus->xfer(d->bus->ctx, tok, NULL, 2);           /* Nwr ≥ 1 байт + start token */
  d->bus->xfer(d->bus->ctx, buf, NULL, SD_BLOCK);
  d->bus->xfer(d->bus->ctx, crc, NULL, 2);
  uint8_t resp = xchg(d, 0xFF);
  if ((resp & SD_DRESP_MASK) != SD_DRESP_OK) { deselect(d); return ST_EVERIFY; }
  for (unsigned i = 0; i < BUSY_POLL_BYTES; i++) {   /* DO = 0 поки карта пише */
    if (xchg(d, 0xFF) == 0xFF) { deselect(d); return ST_OK; }
  }
  deselect(d);
  return ST_ETIMEOUT;
}

const char *sd_type_str(sd_type_t t)
{
  switch (t) {
    case SD_V1_SDSC: return "SDv1";
    case SD_V2_SDSC: return "SDSC";
    case SD_V2_SDHC: return "SDHC";
    default:         return "none";
  }
}
