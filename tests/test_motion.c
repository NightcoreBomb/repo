/**
 * Тести App/motion.c: atan2 у фіксованій точці, кути з акселерометра (FR-02),
 * комплементарний фільтр, dead-zone (FR-04), обрізання (FR-05), обмеження швидкості.
 */
#include <math.h>
#include "unity.h"
#include "motion.h"

void setUp(void) { }
void tearDown(void) { }

/* Повний оберт з кроком 1°: похибка апроксимації ≤ 0.25° (25 cdeg) */
static void test_atan2_accuracy_full_circle(void)
{
  int32_t worst = 0;
  for (int d = -179; d <= 180; d++) {
    double r = d * M_PI / 180.0;
    int32_t y = (int32_t)lround(1000.0 * sin(r)), x = (int32_t)lround(1000.0 * cos(r));
    int32_t got = atan2_cdeg(y, x);
    int32_t ref = (int32_t)lround(atan2((double)y, (double)x) * 18000.0 / M_PI);
    int32_t e = got - ref;
    if (e > 18000) e -= 36000;
    if (e < -18000) e += 36000;
    if (e < 0) e = -e;
    if (e > worst) worst = e;
  }
  TEST_ASSERT_TRUE_MESSAGE(worst <= 25, "atan2 error > 0.25 deg");
}

static void test_atan2_special_cases(void)
{
  TEST_ASSERT_EQUAL_INT32(0, atan2_cdeg(0, 0));
  TEST_ASSERT_EQUAL_INT32(9000, atan2_cdeg(1000, 0));
  TEST_ASSERT_EQUAL_INT32(-9000, atan2_cdeg(-1000, 0));
  TEST_ASSERT_EQUAL_INT32(18000, atan2_cdeg(0, -5));
  TEST_ASSERT_EQUAL_INT32(4500, atan2_cdeg(2000000, 2000000));   /* великі значення — масштабування */
}

static mpu6050_data_t accel(int32_t ax, int32_t ay, int32_t az)
{
  mpu6050_data_t d = { ax, ay, az, 0, 0, 0, 2400 };
  return d;
}

/* FR-02: точки плану тестування PRD — 0°, 30°, 45°, −30° (≤ 3°) */
static void test_accel_angles_prd_points_fr02(void)
{
  static const int deg[] = { 0, 30, 45, -30 };
  for (unsigned i = 0; i < 4; i++) {
    double r = deg[i] * M_PI / 180.0;
    mpu6050_data_t d = accel((int32_t)lround(-1000 * sin(r)), 0, (int32_t)lround(1000 * cos(r)));
    int32_t p, rl;
    att_accel_angles(&d, &p, &rl);
    TEST_ASSERT_INT_WITHIN(300, deg[i] * 100, p);
    TEST_ASSERT_INT_WITHIN(30, 0, rl);
    mpu6050_data_t e = accel(0, (int32_t)lround(1000 * sin(r)), (int32_t)lround(1000 * cos(r)));
    att_accel_angles(&e, &p, &rl);
    TEST_ASSERT_INT_WITHIN(300, deg[i] * 100, rl);
  }
}

static void test_filter_first_sample_and_convergence(void)
{
  attitude_t a;
  att_reset(&a);
  mpu6050_data_t d = accel(-500, 0, 866);                /* 30° */
  att_update(&a, &d, 0);
  TEST_ASSERT_INT_WITHIN(30, 3000, att_pitch_cd(&a));     /* перший відлік — з акселерометра */
  mpu6050_data_t z = accel(0, 0, 1000);
  for (int i = 0; i < 400; i++) att_update(&a, &z, 5000);   /* 2 с у спокої */
  TEST_ASSERT_INT_WITHIN(50, 0, att_pitch_cd(&a));
}

/* Гіроскоп: 90 °/с протягом 0.1 с короткочасно дає ≈ +8.8° (α=0.98 тягне до акселерометра) */
static void test_filter_integrates_gyro(void)
{
  attitude_t a;
  att_reset(&a);
  mpu6050_data_t d = accel(0, 0, 1000);
  att_update(&a, &d, 0);
  d.gy_mdps = 90000;
  for (int i = 0; i < 20; i++) att_update(&a, &d, 5000);
  int32_t p = att_pitch_cd(&a);
  TEST_ASSERT_TRUE(p > 700 && p < 900);
  d.gy_mdps = 0;
  d.gx_mdps = 0;
  a.bias_mdps[1] = 1000;                                  /* calib: зсув нуля віднімається */
  int32_t before = att_pitch_cd(&a);
  att_update(&a, &d, 80000 * 2);                          /* dt обрізається до 80 мс */
  TEST_ASSERT_TRUE(att_pitch_cd(&a) < before);
}

/* FR-04 / FR-05 / обмеження швидкості */
static void test_ctrl_deadzone_clamp_rate(void)
{
  servo_axis_t s = { 9000 };
  TEST_ASSERT_FALSE(ctrl_axis_update(&s, 9500, 1000, 5000));   /* 5° < dz 10° */
  TEST_ASSERT_EQUAL_INT32(9000, s.out_cd);
  TEST_ASSERT_TRUE(ctrl_axis_update(&s, 12000, 0, 5000));
  TEST_ASSERT_EQUAL_INT32(9180, s.out_cd);                      /* 360 °/с × 5 мс = 1.8° */
  for (int i = 0; i < 100; i++) ctrl_axis_update(&s, 99999, 0, 5000);
  TEST_ASSERT_EQUAL_INT32(18000, s.out_cd);                     /* FR-05: не більше 180° */
  for (int i = 0; i < 200; i++) ctrl_axis_update(&s, -99999, 0, 5000);
  TEST_ASSERT_EQUAL_INT32(0, s.out_cd);
  servo_axis_t t = { 0 };
  ctrl_axis_update(&t, 5, 0, 10);                               /* мінімальний крок = 1 cdeg */
  TEST_ASSERT_EQUAL_INT32(1, t.out_cd);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_atan2_accuracy_full_circle);
  RUN_TEST(test_atan2_special_cases);
  RUN_TEST(test_accel_angles_prd_points_fr02);
  RUN_TEST(test_filter_first_sample_and_convergence);
  RUN_TEST(test_filter_integrates_gyro);
  RUN_TEST(test_ctrl_deadzone_clamp_rate);
  return UNITY_END();
}
