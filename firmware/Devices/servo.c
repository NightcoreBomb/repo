#include "servo.h"

uint16_t servo_clamp_us(int32_t us)
{
  if (us < (int32_t)SERVO_US_MIN) return SERVO_US_MIN;
  if (us > (int32_t)SERVO_US_MAX) return SERVO_US_MAX;
  return (uint16_t)us;
}

uint16_t servo_cdeg_to_us(int32_t cdeg)
{
  if (cdeg < 0) cdeg = 0;
  if (cdeg > 18000) cdeg = 18000;
  /* 2000 мкс на 180° → 1 мкс = 0.09°; округлення до найближчого */
  return servo_clamp_us((int32_t)SERVO_US_MIN + (cdeg * 2000 + 9000) / 18000);
}

int32_t servo_us_to_cdeg(uint16_t us)
{
  int32_t u = servo_clamp_us(us);
  return ((u - (int32_t)SERVO_US_MIN) * 18000 + 1000) / 2000;
}
