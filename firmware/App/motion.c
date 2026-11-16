#include "motion.h"

static uint32_t uabs(int32_t v) { return (uint32_t)(v < 0 ? -v : v); }

/* atan(z), z ∈ [0,1]: π/4·z + 0.273·z·(1−z) рад (Rajan et al., 2006) →
   в градусах 45·z + 15.64·z·(1−z); z у Q15. Решта октантів — симетрією. */
int32_t atan2_cdeg(int32_t y, int32_t x)
{
  uint32_t ax = uabs(x), ay = uabs(y);
  if (ax == 0 && ay == 0) return 0;
  uint32_t mn = ax < ay ? ax : ay, mx = ax < ay ? ay : ax;
  while (mn > 65535u) { mn >>= 1; mx >>= 1; }          /* щоб mn<<15 вмістився у 32 біти */
  uint32_t z = (mn << 15) / mx;                         /* 0..32768 */
  int32_t a = (int32_t)((4500u * z + ((1564u * z) >> 15) * (32768u - z)) >> 15);
  if (ay > ax) a = 9000 - a;
  if (x < 0)   a = 18000 - a;
  return y < 0 ? -a : a;
}

void att_reset(attitude_t *a)
{
  a->pitch_md = a->roll_md = 0;
  a->bias_mdps[0] = a->bias_mdps[1] = a->bias_mdps[2] = 0;   /* test_filter_*: без цього — сміття */
  a->init = false;
}

void att_accel_angles(const mpu6050_data_t *d, int32_t *pitch_cd, int32_t *roll_cd)
{
  uint32_t yz = (uint32_t)(d->ay_mg * d->ay_mg) + (uint32_t)(d->az_mg * d->az_mg);
  *pitch_cd = atan2_cdeg(-d->ax_mg, (int32_t)isqrt_u32(yz));
  *roll_cd  = atan2_cdeg(d->ay_mg, d->az_mg);
}

void att_update(attitude_t *a, const mpu6050_data_t *d, uint32_t dt_us)
{
  int32_t p_cd, r_cd;
  att_accel_angles(d, &p_cd, &r_cd);
  if (!a->init) {                                        /* перший відлік — довіряємо акселерометру */
    a->pitch_md = p_cd * 10;
    a->roll_md  = r_cd * 10;
    a->init = true;
    return;
  }
  if (dt_us > MOTION_DT_MAX_US) dt_us = MOTION_DT_MAX_US;
  int32_t dt10 = (int32_t)(dt_us / 10u);
  /* gyro Y ↔ pitch, gyro X ↔ roll; |mdps| ≤ 250 000 → добуток ≤ 2e9 */
  int32_t dp = ((d->gy_mdps - a->bias_mdps[1]) * dt10) / 100000;
  int32_t dr = ((d->gx_mdps - a->bias_mdps[0]) * dt10) / 100000;
  a->pitch_md = (MOTION_ALPHA_PCT * (a->pitch_md + dp) + (100 - MOTION_ALPHA_PCT) * p_cd * 10) / 100;
  a->roll_md  = (MOTION_ALPHA_PCT * (a->roll_md + dr) + (100 - MOTION_ALPHA_PCT) * r_cd * 10) / 100;
}

static int32_t md_to_cd(int32_t md) { return (md >= 0 ? md + 5 : md - 5) / 10; }
int32_t att_pitch_cd(const attitude_t *a) { return md_to_cd(a->pitch_md); }
int32_t att_roll_cd(const attitude_t *a)  { return md_to_cd(a->roll_md); }

bool ctrl_axis_update(servo_axis_t *s, int32_t target_cd, int32_t dz_cd, uint32_t dt_us)
{
  if (target_cd < 0) target_cd = 0;                      /* FR-05: 0..180° */
  if (target_cd > 18000) target_cd = 18000;
  int32_t err = target_cd - s->out_cd;
  if ((int32_t)uabs(err) <= dz_cd) return false;         /* FR-04: dead-zone */
  int32_t step = (int32_t)(((uint32_t)CTRL_RATE_CDEG_S / 100u) * (dt_us / 100u) / 100u);
  if (step < 1) step = 1;
  if (err > step) err = step;
  if (err < -step) err = -step;
  s->out_cd += err;
  return err != 0;
}
