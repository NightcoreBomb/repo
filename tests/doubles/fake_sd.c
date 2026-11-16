#include <string.h>
#include "fake_sd.h"
#include "sdcard.h"

fake_sd_t fake_sd;

void fake_sd_reset(void)
{
  memset(&fake_sd, 0, sizeof fake_sd);
  fake_sd.sdhc = true;
  fake_sd.acmd41_busy = 3;
  fake_sd.fail_read_blk = -1;
  fake_sd.mem[0][510] = 0x55; fake_sd.mem[0][511] = 0xAA;
}

static void push(uint8_t b) { fake_sd.q[fake_sd.qt++ & 15] = b; }

static uint32_t blk_of(uint32_t arg) { return fake_sd.sdhc ? arg : arg / 512; }
static uint8_t *blk_ptr(uint32_t b)
{
  if (b < 8) return fake_sd.mem[b];
  if (b >= 2048 && b < 2048 + 40) return fake_sd.hi[b - 2048];
  return 0;
}

static void process(void)
{
  uint8_t c = fake_sd.cmd[0] & 0x3F;
  uint32_t arg = ((uint32_t)fake_sd.cmd[1] << 24) | ((uint32_t)fake_sd.cmd[2] << 16) |
                 ((uint32_t)fake_sd.cmd[3] << 8) | fake_sd.cmd[4];
  bool app = fake_sd.app;
  fake_sd.app = false;
  uint8_t idle = fake_sd.ready ? 0x00 : 0x01;
  push(0xFF);                                              /* NCR = 1 байт */
  if ((c == 0 || c == 8) && (uint8_t)((sd_crc7(fake_sd.cmd, 5) << 1) | 1) != fake_sd.cmd[5]) {
    fake_sd.bad_crc_seen++;
    push(0x08 | idle);                                     /* com CRC error */
    return;
  }
  switch (c) {
    case 0:  fake_sd.idle = true; fake_sd.ready = false; push(0x01); break;
    case 8:
      if (fake_sd.v1) { push(0x05); break; }               /* idle + illegal */
      push(idle); push(0x00); push(0x00); push(0x01); push(fake_sd.cmd[4]);
      break;
    case 55: fake_sd.app = true; push(idle); break;
    case 41:
      if (!app) { push(0x04 | idle); break; }
      if (fake_sd.acmd41_busy > 0) { fake_sd.acmd41_busy--; push(0x01); }
      else { fake_sd.ready = true; fake_sd.idle = false; push(0x00); }
      break;
    case 58:
      push(idle); push(fake_sd.sdhc ? 0xC0 : 0x80); push(0xFF); push(0x80); push(0x00);
      break;
    case 16: push(idle); break;
    case 17: {
      uint32_t b = blk_of(arg);
      if (!fake_sd.ready || !blk_ptr(b)) { push(0x40 | idle); break; }   /* parameter error */
      if ((int)b == fake_sd.fail_read_blk) { push(0x08); break; }
      push(0x00);
      fake_sd.wstate = 10; fake_sd.wblk = b; fake_sd.widx = -2;     /* -2: пауза, -1: токен */
      break;
    }
    case 24: {
      uint32_t b = blk_of(arg);
      if (!fake_sd.ready || !blk_ptr(b)) { push(0x40 | idle); break; }
      fake_sd.writes++;
      push(0x00);
      fake_sd.wstate = 1; fake_sd.wblk = b; fake_sd.widx = 0;
      break;
    }
    default: push(0x04 | idle);
  }
}

static uint8_t byte(uint8_t mo)
{
  if (fake_sd.absent) return 0xFF;
  if (fake_sd.qh != fake_sd.qt) return fake_sd.q[fake_sd.qh++ & 15];

  if (fake_sd.wstate == 10) {                               /* читання блоку */
    int i = fake_sd.widx++;
    if (i == -2) return 0xFF;
    if (i == -1) return SD_TOKEN_START;
    if (i < 512) return blk_ptr(fake_sd.wblk)[i];
    if (i < 513) return 0x12;                               /* CRC16 (не перевіряється) */
    fake_sd.wstate = 0;
    return 0x34;
  }
  if (fake_sd.wstate == 1) {                                /* очікуємо start token */
    if (mo == SD_TOKEN_START) { fake_sd.wstate = 2; fake_sd.widx = 0; }
    return 0xFF;
  }
  if (fake_sd.wstate == 2) {                                /* 512 даних + 2 CRC */
    fake_sd.wbuf[fake_sd.widx++] = mo;
    if (fake_sd.widx == 514) {
      fake_sd.wstate = 0;
      if (fake_sd.reject_write) { push(0x0D); return 0xFF; }
      memcpy(blk_ptr(fake_sd.wblk), fake_sd.wbuf, 512);
      push(0x05); push(0x00); push(0x00); push(0x00);       /* accepted + busy */
    }
    return 0xFF;
  }
  if (fake_sd.ncmd == 0 && (mo & 0xC0) != 0x40) return 0xFF;
  fake_sd.cmd[fake_sd.ncmd++] = mo;
  if (fake_sd.ncmd == 6) { fake_sd.ncmd = 0; process(); }
  return 0xFF;
}

static void cs(bool low)
{
  if (!low) { fake_sd.ncmd = 0; fake_sd.qh = fake_sd.qt = 0; if (fake_sd.wstate == 10) fake_sd.wstate = 0; }
}

const spi_device_t fake_sd_dev = { cs, byte };
