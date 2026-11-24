#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fake_plat.h"
#include "plat.h"

char     fp_log[65536];
static size_t fp_len;
uint32_t fp_cyc, fp_cyc_step;
uint16_t fp_servo[2];
int      fp_pot;
int      fp_btn_level;
uint32_t fp_btn_edges, fp_btn_first_edge;
int      fp_mark_edges, fp_probe_edges[4];
int      fp_i2c_fault;
const char *fp_console_line;
uint32_t fp_console_eol;
static bool probe[4];

void fake_plat_reset(void)
{
  fp_log[0] = 0; fp_len = 0; fp_cyc = 1; fp_cyc_step = 1;
  fp_servo[0] = fp_servo[1] = 0;
  fp_pot = 2048; fp_btn_level = 1; fp_btn_edges = 0; fp_btn_first_edge = 0; fp_mark_edges = 0;
  memset(fp_probe_edges, 0, sizeof fp_probe_edges); memset(probe, 0, sizeof probe);
  fp_i2c_fault = 0; fp_console_line = NULL; fp_console_eol = 0;
}

void fake_plat_log_rewind(void) { fp_log[0] = 0; fp_len = 0; }

void fp_advance_ms(uint32_t ms) { fp_cyc += ms * 1000u; }

/* ---- час ---- */
uint32_t plat_millis(void) { return fp_cyc / 1000u; }
void plat_delay_ms(uint32_t ms) { fp_advance_ms(ms); }
uint32_t plat_cycles(void) { fp_cyc += fp_cyc_step; return fp_cyc; }
uint32_t plat_cyc_per_us(void) { return 1; }
uint32_t plat_us(void) { return fp_cyc; }

/* ---- консоль ---- */
void plat_log(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(fp_log + fp_len, sizeof fp_log - fp_len, fmt, ap);
  va_end(ap);
  if (n > 0) fp_len += (size_t)n;
  if (fp_len >= sizeof fp_log) fp_len = sizeof fp_log - 1;
}
bool plat_console_getline(char *buf, size_t size, uint32_t *t)
{
  if (!fp_console_line) return false;
  snprintf(buf, size, "%s", fp_console_line);
  fp_console_line = NULL;
  if (t) *t = fp_console_eol;
  return true;
}
uint32_t plat_console_rx_dropped(void) { return 0; }
uint32_t plat_console_rx_overrun(void) { return 0; }
uint32_t plat_console_rx_bytes(void) { return 0; }

/* ---- GPIO / PWM / АЦП ---- */
void plat_probe(unsigned n, bool on)
{
  if (n < 4) { if (on != probe[n]) fp_probe_edges[n]++; probe[n] = on; }
  if (n == PROBE_MARK && on != probe[n]) fp_mark_edges++;
}
void plat_mark(bool l) { if (l != probe[PROBE_MARK]) fp_mark_edges++; plat_probe(PROBE_MARK, l); }
void plat_led_toggle(void) { }
int plat_btn_level(void) { return fp_btn_level; }
uint32_t plat_btn_edges(void) { return fp_btn_edges; }
uint32_t plat_btn_first_edge_cyc(void) { return fp_btn_first_edge; }
void plat_servo_set_us(unsigned ch, uint16_t us) { fp_servo[ch & 1] = us; }
uint16_t plat_servo_get_us(unsigned ch) { return fp_servo[ch & 1]; }
int plat_pot_read_raw(void) { return fp_pot; }

/* ---- I²C діагностика ---- */
uint32_t plat_i2c_recoveries(void) { return 0; }
uint32_t plat_i2c_timeouts(void) { return 0; }
void plat_i2c_fault(int m) { fp_i2c_fault = m; }
int  plat_i2c_fault_get(void) { return fp_i2c_fault; }

/* ---- пам'ять / система ---- */
void plat_stack_paint(void) { }
uint32_t plat_stack_used(void) { return 640; }
uint32_t plat_stack_size(void) { return 1024; }
uint32_t plat_ram_static(void) { return 4096; }
void plat_reset(void) { abort(); }
const char *plat_board_name(void) { return "HOST-FAKE"; }
uint32_t plat_sysclk_hz(void) { return 48000000u; }
uint32_t plat_reset_cause(void) { return 0; }

/* ---- пошук у лозі ---- */
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

int fp_count(const char *prefix)
{
  int c = 0;
  const char *p = fp_log;
  size_t n = strlen(prefix);
  while (p && *p) {
    if (!strncmp(p, prefix, n)) c++;
    p = strchr(p, '\n');
    if (p) p++;
  }
  return c;
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

void plat_console_set_waiter(void *task) { (void)task; }
