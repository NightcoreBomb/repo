#include <string.h>
#include "fake_w25q.h"

fake_w25q_t fake_w25q;

void fake_w25q_reset(void)
{
  memset(&fake_w25q, 0, sizeof fake_w25q);
  memset(fake_w25q.mem, 0xFF, sizeof fake_w25q.mem);
  fake_w25q.jedec = 0xEF4017;
  fake_w25q.busy_after_op = 2;
}

static uint8_t byte(uint8_t mo)
{
  fake_w25q_t *f = &fake_w25q;
  int i = f->idx++;
  if (i == 0) { f->cmd = mo; f->addr = 0;
    if (mo == 0x06 && !f->wel_stuck_low) f->wel = true;
    return 0xFF; }
  switch (f->cmd) {
    case 0x9F: return (uint8_t)(f->jedec >> (8 * (3 - i)));
    case 0x05: {
      uint8_t sr = (uint8_t)((f->wel ? 2 : 0) | (f->busy_reads > 0 ? 1 : 0));
      if (f->busy_reads > 0) f->busy_reads--;
      return sr;
    }
    case 0x03: case 0x02: case 0x20:
      if (i <= 3) { f->addr = (f->addr << 8) | mo; return 0xFF; }
      if (f->cmd == 0x03) return f->mem[(f->addr + (uint32_t)(i - 4)) % sizeof f->mem];
      if (f->cmd == 0x02 && f->wel) {
        uint32_t page = f->addr & ~0xFFu, off = (f->addr + (uint32_t)(i - 4)) & 0xFF;
        f->mem[(page + off) % sizeof f->mem] &= mo;        /* NOR: 1→0 */
      }
      return 0xFF;
    default: return 0xFF;
  }
}

static void cs(bool low)
{
  fake_w25q_t *f = &fake_w25q;
  if (low) { f->idx = 0; return; }
  /* CS↑ завершує команду: PP/SE виконуються лише при WEL=1 */
  if ((f->cmd == 0x02 || f->cmd == 0x20) && f->idx >= 4) {
    if (!f->wel) f->ignored_programs++;
    else {
      if (f->cmd == 0x20) memset(&f->mem[(f->addr & ~0xFFFu) % sizeof f->mem], 0xFF, 4096);
      f->wel = false;
      f->busy_reads = f->busy_after_op;
    }
  }
}

const spi_device_t fake_w25q_dev = { cs, byte };
