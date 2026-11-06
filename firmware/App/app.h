/**
 * app.h — прикладний шар: self-test, консоль, сценарії запису для аналізатора.
 * Бачить лише Interfaces/ і Devices/ — жодного HAL (перевіряє tools/check_deps.py).
 */
#ifndef APP_APP_H
#define APP_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "bus_i2c.h"
#include "bus_spi.h"
#include "mpu6050.h"
#include "ssd1306.h"
#include "sdcard.h"

#define APP_FW_TAG   "LR2"
#define APP_TICK_MS  1000u

typedef struct {
  const bus_i2c_t *i2c;
  const bus_spi_t *sd_spi;
  mpu6050_t imu;
  ssd1306_t oled;
  sdcard_t  sd;
  bool imu_ok, oled_ok, sd_ok;
  uint32_t next_tick, ticks, last_presses;
} app_t;

void app_init(app_t *a, const bus_i2c_t *i2c, const bus_spi_t *sd_spi);
void app_poll(app_t *a);                       /* викликати з while(1) у main */
void app_handle_line(app_t *a, char *line);    /* розбір однієї команди консолі */

#endif
