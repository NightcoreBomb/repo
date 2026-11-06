#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "fake_plat.h"
#include "plat.h"

char     fp_log[16384];
static size_t fp_len;
uint32_t fp_now;
uint16_t fp_servo[2];
int      fp_pot;
int      fp_btn_level;
uint32_t fp_btn_presses;
int      fp_mark_edges;
bool     fp_restart;
const char *fp_console_line;
static bool mark;

void fake_plat_reset(void)
{
  fp_log[0] = 0; fp_len = 0; fp_now = 0; fp_servo[0] = fp_servo[1] = 0;
  fp_pot = 2048; fp_btn_level = 1; fp_btn_presses = 0; fp_mark_edges = 0;
  fp_restart = false; fp_console_line = NULL; mark = false;
}

uint32_t plat_millis(void) { return fp_now++; }      /* кожне читання — +1 мс */
void plat_delay_ms(uint32_t ms) { fp_now += ms; }
void plat_log(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(fp_log + fp_len, sizeof fp_log - fp_len, fmt, ap);
  va_end(ap);
  if (n > 0) fp_len += (size_t)n;
  if (fp_len >= sizeof fp_log) fp_len = sizeof fp_log - 1;
}
void plat_mark(bool l) { if (l != mark) fp_mark_edges++; mark = l; }
void plat_servo_set_us(unsigned ch, uint16_t us) { fp_servo[ch & 1] = us; }
uint16_t plat_servo_get_us(unsigned ch) { return fp_servo[ch & 1]; }
int plat_pot_read_raw(void) { return fp_pot; }
int plat_btn_level(void) { return fp_btn_level; }
uint32_t plat_btn_presses(void) { return fp_btn_presses; }
void plat_led_toggle(void) { }
bool plat_console_getline(char *buf, size_t size)
{
  if (!fp_console_line) return false;
  snprintf(buf, size, "%s", fp_console_line);
  fp_console_line = NULL;
  return true;
}
void plat_i2c_restart(bool on) { fp_restart = on; }
const char *plat_board_name(void) { return "HOST-FAKE"; }
uint32_t plat_sysclk_hz(void) { return 48000000u; }

const char *fp_find(const char *prefix)
{
  const char *p = fp_log;
  size_t n = strlen(prefix);
  while (p && *p) {
    if (!strncmp(p, prefix, n)) return p;
    p = strchr(p, '\n');
    if (p) p++;
  }
  return NULL;
}

const char *fp_kv(const char *line, const char *key, char *out, int max)
{
  char pat[40];
  snprintf(pat, sizeof pat, " %s=", key);
  const char *eol = strchr(line, '\n');
  const char *p = strstr(line, pat);
  if (!p || (eol && p > eol)) return NULL;
  p += strlen(pat);
  int i = 0;
  while (p[i] && p[i] != ' ' && p[i] != '\r' && p[i] != '\n' && i < max - 1) { out[i] = p[i]; i++; }
  out[i] = 0;
  return out;
}
