/**
 * motion.h — орієнтація (комплементарний фільтр) і закон керування серво.
 * Чиста цілочисельна логіка (Cortex-M0+ без FPU) — тестується на хості.
 *
 * Одиниці: кути — соті градуса (cdeg) на виході, тисячні (mdeg) усередині фільтра;
 * прискорення — mg, кутова швидкість — mdps (як у Devices/mpu6050.h).
 */
#ifndef APP_MOTION_H
#define APP_MOTION_H

#include <stdbool.h>
#include <stdint.h>
#include "mpu6050.h"

#define MOTION_ALPHA_PCT     98      /* вага гіроскопа у фільтрі (τ ≈ 0.25 с @200 Гц) */
#define MOTION_DT_MAX_US     80000u  /* довший інтервал (пропущені запуски) — обрізається */
#define CTRL_RATE_CDEG_S     36000   /* обмеження швидкості серво: 360 °/с */

/* atan2 у сотих градуса, −18000..18000; похибка апроксимації ≤ 0.25° */
int32_t atan2_cdeg(int32_t y, int32_t x);

typedef struct {
  int32_t pitch_md, roll_md;         /* оцінка фільтра, мілліградуси */
  int32_t bias_mdps[3];              /* зсув нуля гіроскопа (команда calib) */
  bool    init;
} attitude_t;

void att_reset(attitude_t *a);
/* Кути лише з акселерометра (статика) */
void att_accel_angles(const mpu6050_data_t *d, int32_t *pitch_cd, int32_t *roll_cd);
/* Крок фільтра: dt — фактичний інтервал між вимірами */
void att_update(attitude_t *a, const mpu6050_data_t *d, uint32_t dt_us);
int32_t att_pitch_cd(const attitude_t *a);
int32_t att_roll_cd(const attitude_t *a);

/* Закон керування однією віссю серво (FR-03..FR-05) */
typedef struct {
  int32_t out_cd;                    /* поточний кут серво 0..18000 */
} servo_axis_t;

/* Ціль 0..18000 (обрізається, FR-05); зміни, менші за dz_cd, ігноруються (FR-04);
   крок не більший за CTRL_RATE_CDEG_S·dt. Повертає true, якщо вихід змінився. */
bool ctrl_axis_update(servo_axis_t *s, int32_t target_cd, int32_t dz_cd, uint32_t dt_us);

#endif
