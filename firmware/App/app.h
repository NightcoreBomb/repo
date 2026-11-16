/**
 * app.h — прикладний шар ЛР3: продукт у моделі super loop + вимірювання.
 * Бачить лише Interfaces/ і Devices/ — жодного HAL (перевіряє tools/check_deps.py).
 *
 * Структура головного циклу (одна ітерація app_poll):
 *   cmd → btn → imu(+ctrl) → pot → rec | play → oled → telem → load
 * Кожна задача перевіряє, чи настав її час (мітки часу), і повертає керування;
 * термінове — у перериваннях: SysTick (1 кГц, debounce), EXTI8 (фронти кнопки),
 * USART2 RX (байти консолі в кільце). Опис — docs/tasks.md, docs/lab3-report.md §2.
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
#include "button.h"
#include "motion.h"
#include "perf.h"
#include "traj.h"

#define APP_FW_TAG        "LR3"

/* Періоди задач (docs/tasks.md) */
#define IMU_PERIOD_US     5000u     /* FR-01: 200 Гц */
#define POT_PERIOD_MS     50u       /* FR-04 */
#define OLED_PERIOD_MS    200u      /* FR-10: ≥ 5 Гц */
#define TELEM_PERIOD_MS   100u      /* FR-11: 10 Гц за замовчуванням */
#define REC_PERIOD_MS     TRAJ_PERIOD_MS   /* FR-07/08: 50 Гц */
#define REC_GAP_MS        25u       /* FR-07: інтервал між записами > 25 мс = пропуск */

typedef enum { MODE_LIVE = 0, MODE_REC, MODE_PLAY } app_mode_t;

/* Задачі та блокувальні виклики, для яких ведеться STAT */
enum {
  T_LOOP = 0, T_CMD, T_BTN, T_IMU, T_POT, T_REC, T_PLAY, T_OLED, T_TELEM,
  B_I2C_BURST, B_CTRL, B_SD_WRITE, B_SD_READ, B_OLED_TXN, B_UART_LINE, B_LONG,
  T_COUNT
};
enum { L_BTN_DB = 0, L_BTN_EDGE, L_CMD, L_CTRL, L_COUNT };

typedef struct {
  const bus_i2c_t *i2c;
  const bus_spi_t *sd_spi;
  mpu6050_t imu;
  ssd1306_t oled;
  sdcard_t  sd;
  bool imu_ok, oled_ok, sd_ok;

  app_mode_t mode;
  attitude_t att;
  servo_axis_t pan, tilt;
  int32_t  dz_cd;                       /* dead-zone з потенціометра */
  int32_t  pitch_cd, roll_cd;           /* остання оцінка (або підстановка `angle`) */
  bool     ovr;                         /* `angle p r` — підстановка кутів (FR-05) */
  int32_t  ovr_pitch, ovr_roll;
  int      imu_status;
  uint32_t imu_errors, imu_samples;

  /* розклад: IMU — у тактах (точність), решта — у мс */
  uint32_t imu_period_us, imu_next, imu_prev;
  uint32_t t_pot, t_oled, t_telem, t_rec;
  uint16_t telem_ms;                    /* 0 — телеметрію вимкнено */
  uint8_t  oled_mode, oled_step;        /* 0 — блокувально все; 1 — рядок; 2 — транзакція */
  uint32_t oled_frames, oled_frames_t0;
  bool     evt;
  uint32_t busy_us;                     /* `load N`: штучна робота в циклі (точка зламу) */

  /* журнал */
  traj_enc_t enc;
  traj_dir_t dir;
  bool       dir_ok;
  uint32_t rec_t0, rec_prev, rec_records, rec_gaps, rec_max_int;
  uint16_t rec_blocks;
  uint16_t play_blk, play_i, play_n;
  int16_t  play_pan, play_tilt;
  uint32_t play_t0, play_records;
  bool     play_done;
  uint8_t  blk[TRAJ_BLOCK];

  /* вимірювання */
  perf_stat_t st[T_COUNT];
  perf_stat_t lat[L_COUNT];
  perf_jit_t  jit_imu;
  perf_load_t load;
  uint32_t loop_prev;
  uint32_t first_imu_us;
} app_t;

void app_init(app_t *a, const bus_i2c_t *i2c, const bus_spi_t *sd_spi);
void app_start(app_t *a, bool adc_ok);         /* банер, FR-12, каталог журналу (FR-09) */
void app_poll(app_t *a);                       /* одна ітерація super loop */
void app_handle_line(app_t *a, char *line);    /* одна команда консолі */
void app_set_mode(app_t *a, app_mode_t m);
void app_stats(app_t *a);
void app_stats_reset(app_t *a);
const char *app_mode_str(app_mode_t m);

/* SysTick 1 кГц: debounce кнопки MODE (FR-06) */
void     app_tick_1ms(void);
uint32_t app_btn_presses(void);
int      app_btn_stable_level(void);

#endif
