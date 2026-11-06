/**
 * fake_plat — FAKE сервісів платформи (Interfaces/plat.h) для тестів App/:
 * лог збирається в буфер, час — віртуальний, серво/АЦП/кнопка — змінні.
 */
#ifndef FAKE_PLAT_H
#define FAKE_PLAT_H
#include <stdint.h>
#include <stdbool.h>

extern char     fp_log[16384];
extern uint32_t fp_now;
extern uint16_t fp_servo[2];
extern int      fp_pot;
extern int      fp_btn_level;
extern uint32_t fp_btn_presses;
extern int      fp_mark_edges;
extern bool     fp_restart;
extern const char *fp_console_line;

void fake_plat_reset(void);
/* знайти рядок логу, що починається з prefix; NULL якщо нема */
const char *fp_find(const char *prefix);
/* значення key=... у рядку line (до пробілу), або NULL */
const char *fp_kv(const char *line, const char *key, char *out, int max);

#endif
