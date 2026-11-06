/* АВТОГЕНЕРОВАНО tools/vcd2vectors.py — не редагувати вручну.
 *   lr1_bringup_all.vcd: 1 WHO_AM_I, 0 burst, 18 UART-рядків
 */
#ifndef CAPTURE_VECTORS_H
#define CAPTURE_VECTORS_H
#include <stdint.h>

typedef struct { const char *src; double t_ms; uint8_t bus; int uart; } cv_who_t;
static const cv_who_t cv_who[] = {
  { "lr1_bringup_all.vcd", 712.815, 0x68, 0x68 },
};
#define CV_WHO_N 1

typedef struct { const char *src; double t_ms; uint8_t bus[14]; int has_uart; int32_t uart[7]; } cv_burst_t;
/* uart[] = ax ay az (mg), t_cC (0.01 °C), gx gy gz (m°/с) — як надрукувала прошивка */
static const cv_burst_t cv_burst[] = {
  { 0 }
};
#define CV_BURST_N 0

#endif
