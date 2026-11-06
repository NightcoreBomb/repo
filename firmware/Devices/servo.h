/**
 * servo.h — перетворення кута сервоприводу в ширину імпульсу і назад.
 * SG90: 50 Гц, 500 мкс ≈ 0°, 2500 мкс ≈ 180° (PRD FR-05). Чисті функції.
 */
#ifndef DEV_SERVO_H
#define DEV_SERVO_H

#include <stdint.h>

#define SERVO_US_MIN     500u
#define SERVO_US_MAX     2500u
#define SERVO_US_CENTER  1500u
#define SERVO_PERIOD_US  20000u

/* кут у сотих градуса (0..18000); поза межами — обрізається (FR-05) */
uint16_t servo_cdeg_to_us(int32_t cdeg);
int32_t  servo_us_to_cdeg(uint16_t us);
uint16_t servo_clamp_us(int32_t us);

#endif
