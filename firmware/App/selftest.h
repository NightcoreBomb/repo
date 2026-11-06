/**
 * selftest.h — режим самоперевірки (bring-up).
 *
 * Формат виводу (по рядку на пристрій, придатний для автоматичного розбору,
 * див. tools/parse_selftest.py):
 *   ST BEGIN fw=LR2 board=<name> sysclk=<Hz>
 *   ST <dev> <PASS|FAIL> key=value key=value ...
 *   ST END pass=<n> fail=<n>
 */
#ifndef APP_SELFTEST_H
#define APP_SELFTEST_H

#include <stdint.h>
#include "app.h"

/* Критерії фізичної правдоподібності IMU у спокої */
#define ST_IMU_SAMPLES        8
#define ST_IMU_NORM_MG        1000
#define ST_IMU_NORM_TOL_MG    50      /* 1 g ± 5 % */
#define ST_IMU_GYRO_MAX_MDPS  25000   /* PS §6.1: ZRO tolerance ±20 °/с + запас */
#define ST_IMU_TEMP_MIN_CC    0       /* «кімнатна» температура 0..50 °C */
#define ST_IMU_TEMP_MAX_CC    5000
#define ST_SD_TEST_BLOCK      1u      /* неруйнівний RW-тест: читаємо, пишемо, відновлюємо */

typedef struct { int pass, fail; } st_summary_t;

typedef struct {               /* результат вимірювання IMU (для тестів і imuchk) */
  int status;
  mpu6050_raw_t  raw;          /* останнє сире читання */
  mpu6050_data_t avg;          /* середнє з N читань, фізичні одиниці */
  uint32_t norm_mg;
} imu_sample_t;

st_summary_t selftest_run(app_t *a);
int  selftest_imu(app_t *a);
int  selftest_oled(app_t *a);
int  selftest_sd(app_t *a);
int  selftest_servo(void);
int  selftest_pot(void);
int  selftest_btn(void);

/* Усереднене читання IMU (n ≥ 1) */
void imu_measure(app_t *a, int n, imu_sample_t *out);
/* Перевірка показів IMU проти очікуваних (для automation scenario Wokwi) */
int  imu_check(app_t *a, const int32_t exp[7], const int32_t tol[3]);

#endif
