/* АВТОГЕНЕРОВАНО tools/vcd2vectors.py — не редагувати вручну.
 *   lr1_bringup_all.vcd: 1 WHO_AM_I, 0 burst, 18 UART-рядків
 *   lr2_run1_i2c_mpu-init-stall.vcd: 1 WHO_AM_I, 0 burst, 27 UART-рядків
 *   lr2_run2_i2c_oled-clear-stall.vcd: 1 WHO_AM_I, 0 burst, 26 UART-рядків
 *   lr2_run3_i2c_fallback-stall.vcd: 1 WHO_AM_I, 0 burst, 32 UART-рядків
 *   lr2_run4_i2c_second-byte-lost.vcd: 1 WHO_AM_I, 0 burst, 34 UART-рядків
 */
#ifndef CAPTURE_VECTORS_H
#define CAPTURE_VECTORS_H
#include <stdint.h>

typedef struct { const char *src; double t_ms; uint8_t bus; int uart; } cv_who_t;
static const cv_who_t cv_who[] = {
  { "lr1_bringup_all.vcd", 712.815, 0x68, 0x68 },
  { "lr2_run1_i2c_mpu-init-stall.vcd", 48.771, 0x68, -1 },
  { "lr2_run2_i2c_oled-clear-stall.vcd", 55.813, 0x68, -1 },
  { "lr2_run3_i2c_fallback-stall.vcd", 55.829, 0x68, -1 },
  { "lr2_run4_i2c_second-byte-lost.vcd", 55.816, 0x68, -1 },
};
#define CV_WHO_N 5

typedef struct { const char *src; double t_ms; uint8_t bus[14]; int has_uart; int32_t uart[7]; } cv_burst_t;
/* uart[] = ax ay az (mg), t_cC (0.01 °C), gx gy gz (m°/с) — як надрукувала прошивка */
static const cv_burst_t cv_burst[] = {
  { 0 }
};
#define CV_BURST_N 0

#endif
