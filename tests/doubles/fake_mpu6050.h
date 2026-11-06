/**
 * fake_mpu6050 — FAKE: спрощена, але робоча модель чипа на «шині».
 * Має регістровий файл (128 байтів), автоінкремент вказівника, стан SLEEP
 * після POR (PWR_MGMT_1 = 0x40) і віддає нулі в регістрах даних, поки спить —
 * так драйвер, що забув розбудити чип, отримає «живий» провал у тесті.
 * На ту ж шину можна «посадити» SSD1306-заглушку (лише ACK на 0x3C).
 */
#ifndef FAKE_MPU6050_H
#define FAKE_MPU6050_H
#include <stdint.h>
#include "bus_i2c.h"

typedef struct {
  uint8_t regs[128];
  uint8_t ptr;
  uint8_t addr;
  int     present;         /* 0 — на адресу ніхто не відповідає (NACK) */
  int     oled_present;    /* ACK на 0x3C */
  int     force_err;       /* ≠0 — усі транзакції до MPU повертають цей код */
  unsigned writes, reads, oled_writes;
} fake_mpu_t;

extern fake_mpu_t fake_mpu;
extern int fake_mpu_fail_writes;   /* ≠0 — записи ≥2 байтів до MPU повертають цей код */
extern const bus_i2c_t fake_mpu_bus;

void fake_mpu_reset(void);
/* встановити «фізику»: g — у g·1000 (mg), dps — у m°/с, температура — у 0.01 °C */
void fake_mpu_set(int32_t ax_mg, int32_t ay_mg, int32_t az_mg,
                  int32_t gx_mdps, int32_t gy_mdps, int32_t gz_mdps, int32_t t_cC);
void fake_mpu_set_raw16(uint8_t reg, int16_t v);

#endif
