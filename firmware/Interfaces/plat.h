/**
 * plat.h — сервіси платформи для прикладного шару (App/).
 *
 * App бачить лише ці функції, а не HAL. Реалізація на STM32 —
 * Platform/plat_stm32.c; у хостових тестах — tests/doubles/fake_plat.c.
 */
#ifndef IF_PLAT_H
#define IF_PLAT_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ---------- час ---------- */
uint32_t plat_millis(void);
void     plat_delay_ms(uint32_t ms);

/* Лічильник тактів ядра — аналог DWT->CYCCNT (у Cortex-M0+ блоку DWT CYCCNT немає).
   Будується з SysTick: мілісекунди (uwTick) × тактів_на_мс + (LOAD − VAL).
   32 біти, 48 МГц → переповнення кожні 89.5 с; різниця беззнакових значень коректна
   для інтервалів, коротших за цей час. Роздільність — 1 такт (20.8 нс). */
uint32_t plat_cycles(void);
uint32_t plat_cyc_per_us(void);           /* 48 */
/* Мікросекунди від старту (32 біти, переповнення через 71.6 хв) — для міток EVT */
uint32_t plat_us(void);

/* ---------- консоль ---------- */
/* Лог у консоль (UART VCP), БЛОКУВАЛЬНИЙ: повертається, коли останній байт у TDR.
   Формат — підмножина printf: %[-0][width][l]{d,u,x,X,s,c,%} (без float). */
void     plat_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
size_t   plat_vfmt(char *buf, size_t size, const char *fmt, va_list ap);
size_t   plat_fmt(char *buf, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
/* Неблокувальне читання рядка з консолі: true, якщо в buf готовий рядок.
   *t_cyc — момент (plat_cycles), коли ISR прийняв символ кінця рядка. */
bool     plat_console_getline(char *buf, size_t size, uint32_t *t_cyc);
uint32_t plat_console_rx_dropped(void);   /* байтів відкинуто: кільце RX переповнене */
uint32_t plat_console_rx_overrun(void);   /* апаратних ORE (байт не встигли забрати) */
uint32_t plat_console_rx_bytes(void);

/* ---------- GPIO ---------- */
/* Службові піни для логічного аналізатора (docs/pinout.md): 0 IMU, 1 PWM, 2 LONG, 3 MARK */
enum { PROBE_IMU = 0, PROBE_PWM = 1, PROBE_LONG = 2, PROBE_MARK = 3 };
void     plat_probe(unsigned n, bool on);
void     plat_mark(bool level);           /* = plat_probe(PROBE_MARK, level) */
void     plat_led_toggle(void);

/* СИРИЙ рівень входу кнопки MODE (1 — відпущена, pull-up), без debounce. */
int      plat_btn_level(void);
uint32_t plat_btn_edges(void);            /* сирих фронтів (EXTI) — брязкіт */
uint32_t plat_btn_first_edge_cyc(void);   /* такт першого спаду після спокою (EXTI) */

/* ---------- PWM / АЦП ---------- */
void     plat_servo_set_us(unsigned ch, uint16_t us);   /* 0 pan, 1 tilt */
uint16_t plat_servo_get_us(unsigned ch);
int      plat_pot_read_raw(void);         /* 0..4095 або -1 */

/* ---------- I²C: діагностика та інжекція відмов (сценарій 6.2.4) ---------- */
uint32_t plat_i2c_recoveries(void);
uint32_t plat_i2c_timeouts(void);
/* 0 — норма; 1 — «SDA затиснута»: кожна транзакція чекає повний таймаут і
   повертає TIMEOUT (у Wokwi лінію фізично не затиснути — емуляція в драйвері) */
void     plat_i2c_fault(int mode);
int      plat_i2c_fault_get(void);

/* ---------- пам'ять ---------- */
void     plat_stack_paint(void);          /* перший рядок main() */
uint32_t plat_stack_used(void);           /* виміряна глибина (байтів) */
uint32_t plat_stack_size(void);           /* зарезервовано лінкер-скриптом */
uint32_t plat_ram_static(void);           /* .data + .bss */

/* ---------- система ---------- */
void        plat_reset(void) __attribute__((__noreturn__));
const char *plat_board_name(void);
uint32_t    plat_sysclk_hz(void);
uint32_t    plat_reset_cause(void);       /* RCC_CSR: прапорці причини скиду */

#endif
