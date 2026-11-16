/**
 * fake_plat — FAKE сервісів платформи (Interfaces/plat.h) для тестів App/:
 * лог збирається в буфер, час — віртуальний, серво/АЦП/кнопка — змінні.
 * Лічильник тактів: 1 такт = 1 мкс (plat_cyc_per_us() = 1); кожне читання
 * plat_cycles() просуває час на fp_cyc_step тактів (час «іде», поки код працює).
 */
#ifndef FAKE_PLAT_H
#define FAKE_PLAT_H
#include <stdint.h>
#include <stdbool.h>

extern char     fp_log[65536];
extern uint32_t fp_cyc;          /* такти (=мкс); plat_millis() = fp_cyc / 1000 */
extern uint32_t fp_cyc_step;
extern uint16_t fp_servo[2];
extern int      fp_pot;
extern int      fp_btn_level;
extern uint32_t fp_btn_edges, fp_btn_first_edge;
extern int      fp_mark_edges, fp_probe_edges[4];
extern int      fp_i2c_fault;
extern const char *fp_console_line;
extern uint32_t fp_console_eol;

void fake_plat_reset(void);
/* очистити лише лог (час не скидати) */
void fake_plat_log_rewind(void);
/* просунути віртуальний час на ms мілісекунд (і такти разом із ним) */
void fp_advance_ms(uint32_t ms);
/* знайти рядок логу, що починається з prefix; NULL якщо нема */
const char *fp_find(const char *prefix);
/* значення key=... у рядку line (до пробілу), або NULL */
const char *fp_kv(const char *line, const char *key, char *out, int max);
/* кількість рядків логу, що починаються з prefix */
int fp_count(const char *prefix);

#endif
