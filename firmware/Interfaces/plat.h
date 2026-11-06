/**
 * plat.h — сервіси платформи для прикладного шару (App/).
 *
 * App бачить лише ці функції, а не HAL. Реалізація на STM32 —
 * Platform/plat_stm32.c; у хостових тестах — tests/doubles/fake_plat.c.
 */
#ifndef IF_PLAT_H
#define IF_PLAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint32_t plat_millis(void);
void     plat_delay_ms(uint32_t ms);

/* Лог у консоль (UART VCP). Рядок формату — як у printf (без float). */
void     plat_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Службовий GPIO «MARK»: тригер/гейт для логічного аналізатора */
void     plat_mark(bool level);

/* Сервоканали: 0 = pan (TIM3_CH1), 1 = tilt (TIM3_CH2); ширина імпульсу, мкс */
void     plat_servo_set_us(unsigned ch, uint16_t us);
uint16_t plat_servo_get_us(unsigned ch);

/* Одне перетворення АЦП потенціометра; повертає 0..4095 або -1 */
int      plat_pot_read_raw(void);

/* Рівень входу кнопки MODE (1 — відпущена, pull-up) і лічильник натискань */
int      plat_btn_level(void);
uint32_t plat_btn_presses(void);

void     plat_led_toggle(void);

/* Неблокувальне читання рядка з консолі: true, якщо в buf готовий рядок */
bool     plat_console_getline(char *buf, size_t size);

/* Режим I²C write_read: true — repeated START (Sr), false — STOP+START.
   Платформа обирає дефолт (BOARD_I2C_USE_RESTART); App може перемкнути для експерименту. */
void     plat_i2c_restart(bool on);
/* Спосіб запису ≥2 байтів (0 HAL з передзавантаженням, 1 по TXIS, 2 Mem_Write) */
void     plat_i2c_write_mode(int mode);
int      plat_i2c_get_write_mode(void);
/* Скільки разів периферію I²C скидали після TIMEOUT/BUSY */
uint32_t plat_i2c_recoveries(void);

/* Ідентифікація платформи для заголовка self-test */
const char *plat_board_name(void);
uint32_t    plat_sysclk_hz(void);

#endif
