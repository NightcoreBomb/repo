/**
 * mpu6050.h — драйвер InvenSense MPU-6050 (I²C).
 * Залежить лише від Interfaces/bus_i2c.h. Арифметика — цілочисельна
 * (Cortex-M0+ без FPU): прискорення в mg, кутова швидкість у mdps (м°/с),
 * температура в сотих °C.
 *
 * Джерела: PS-MPU-6000A-00 rev 3.4 (PS), RM-MPU-6000A-00 rev 4.2 (RM).
 */
#ifndef DEV_MPU6050_H
#define DEV_MPU6050_H

#include <stdint.h>
#include "bus_i2c.h"

#define MPU6050_ADDR_AD0_LOW   0x68u   /* RM §4.32, PS §9.2 */
#define MPU6050_ADDR_AD0_HIGH  0x69u
#define MPU6050_WHO_AM_I_VAL   0x68u   /* RM §4.32: біти [6:1] = 110100 */

/* Регістри (RM §3, «Register Map») */
#define MPU6050_REG_SMPLRT_DIV    0x19u
#define MPU6050_REG_CONFIG        0x1Au
#define MPU6050_REG_GYRO_CONFIG   0x1Bu
#define MPU6050_REG_ACCEL_CONFIG  0x1Cu
#define MPU6050_REG_ACCEL_XOUT_H  0x3Bu
#define MPU6050_REG_PWR_MGMT_1    0x6Bu
#define MPU6050_REG_WHO_AM_I      0x75u

#define MPU6050_BURST_LEN         14u   /* 0x3B..0x48: accel(6) temp(2) gyro(6) */

typedef enum { MPU_ACCEL_2G = 0, MPU_ACCEL_4G, MPU_ACCEL_8G, MPU_ACCEL_16G } mpu_accel_fs_t;
typedef enum { MPU_GYRO_250 = 0, MPU_GYRO_500, MPU_GYRO_1000, MPU_GYRO_2000 } mpu_gyro_fs_t;

typedef struct {
  const bus_i2c_t *bus;
  uint8_t          addr;
  mpu_accel_fs_t   accel_fs;
  mpu_gyro_fs_t    gyro_fs;
  uint8_t          who_am_i;      /* останнє прочитане значення */
  uint8_t          pwr_mgmt_1;    /* read-back після пробудження */
  uint8_t          fail_reg;      /* регістр, на якому зупинився init (0xFF — немає) */
} mpu6050_t;

typedef struct {               /* сирі значення, як на шині (big-endian → int16) */
  int16_t ax, ay, az, temp, gx, gy, gz;
} mpu6050_raw_t;

typedef struct {               /* фізичні одиниці */
  int32_t ax_mg, ay_mg, az_mg;
  int32_t gx_mdps, gy_mdps, gz_mdps;
  int32_t temp_cC;             /* соті градуса Цельсія */
} mpu6050_data_t;

void mpu6050_setup(mpu6050_t *d, const bus_i2c_t *bus, uint8_t addr);

/* Прочитати WHO_AM_I. ST_EID — якщо відповідь не 0x68. */
int  mpu6050_identify(mpu6050_t *d);

/* Мінімальна ініціалізація до робочого режиму:
   PWR_MGMT_1 = 0x01 (вийти зі sleep, PLL від гіроскопа X — PS §6.3, RM §4.28),
   CONFIG = 0x03 (DLPF 44 Гц), SMPLRT_DIV = 4 (1 кГц/(1+4) = 200 Гц — FR-01),
   GYRO_CONFIG/ACCEL_CONFIG — обрані діапазони. */
int  mpu6050_init(mpu6050_t *d);

/* Одне багатобайтове читання 14 байтів з 0x3B */
int  mpu6050_read_raw(mpu6050_t *d, mpu6050_raw_t *raw);

/* Розбір 14 байтів у тому порядку, у якому вони йдуть по шині */
void mpu6050_parse_burst(const uint8_t b[MPU6050_BURST_LEN], mpu6050_raw_t *raw);

/* Перетворення в фізичні одиниці (чисті функції — тестуються на хості) */
int32_t mpu6050_accel_lsb_per_g(mpu_accel_fs_t fs);       /* 16384/8192/4096/2048 */
int32_t mpu6050_gyro_lsb_per_dps_x10(mpu_gyro_fs_t fs);   /* 1310/655/328/164     */
int32_t mpu6050_accel_to_mg(int16_t raw, mpu_accel_fs_t fs);
int32_t mpu6050_gyro_to_mdps(int16_t raw, mpu_gyro_fs_t fs);
int32_t mpu6050_temp_to_cC(int16_t raw);                  /* raw/340 + 36.53 °C (RM §4.18) */
void    mpu6050_convert(const mpu6050_t *d, const mpu6050_raw_t *raw, mpu6050_data_t *out);

/* Модуль вектора прискорення, mg (цілочисельний sqrt) */
uint32_t mpu6050_accel_norm_mg(const mpu6050_data_t *v);
uint32_t isqrt_u32(uint32_t x);

#endif
