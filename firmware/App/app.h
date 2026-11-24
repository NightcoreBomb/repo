/**
 * app.h — прикладний шар ЛР4: той самий продукт, що в ЛР3, організований задачами FreeRTOS.
 * Бачить лише Interfaces/ і Devices/ — ні HAL, ні заголовків FreeRTOS (tools/check_deps.py);
 * сервіси ядра — через Interfaces/os.h.
 *
 * Задачі (docs/rtos-design.md §1), пріоритет — у дужках:
 *   btn(6)     сповіщення з SysTick tick hook (debounce підтвердив натискання)
 *   imu(5)     vTaskDelayUntil 5 мс: I²C → фільтр → серво; у REC — запис у чергу q_rec
 *   journal(4) черга q_rec → кодування → SD (REC); у PLAY — vTaskDelayUntil 20 мс
 *   cmd(3)     сповіщення з USART2 RX ISR (кінець рядка) → команди
 *   telem(2), oled(2)  періодичні, рівний пріоритет (time slicing)
 *   long_op(1) черга q_long: dump, calib — їх витісняють усі
 * Логіка кроків (*_run) — та сама, що в задачах super loop ЛР3; app_poll() лишився для
 * хостових тестів (APP_COOP): він викликає ті самі кроки за мітками часу.
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
#include "os.h"

#define APP_FW_TAG        "LR4"

/* Періоди (docs/tasks.md) — ті самі, що в ЛР3 */
#define IMU_PERIOD_US     5000u     /* FR-01: 200 Гц */
#define POT_PERIOD_MS     50u       /* FR-04 */
#define OLED_PERIOD_MS    200u      /* FR-10: ≥ 5 Гц */
#define TELEM_PERIOD_MS   100u      /* FR-11 */
#define REC_PERIOD_MS     TRAJ_PERIOD_MS   /* FR-07/08: 50 Гц */
#define REC_GAP_MS        25u       /* FR-07: інтервал між записами > 25 мс = пропуск */

/* Пріоритети задач (configMAX_PRIORITIES = 7) — обґрунтування: docs/rtos-design.md §1 */
enum { PRIO_LONG = 1, PRIO_TELEM = 2, PRIO_OLED = 2, PRIO_CMD = 3, PRIO_JOURNAL = 4,
       PRIO_IMU = 5, PRIO_BTN = 6 };

typedef enum { MODE_LIVE = 0, MODE_REC, MODE_PLAY } app_mode_t;

/* Задачі та блокувальні виклики, для яких ведеться STAT (імена — як у ЛР3 + нові) */
enum {
  T_LOOP = 0, T_CMD, T_BTN, T_IMU, T_POT, T_REC, T_PLAY, T_OLED, T_TELEM,
  B_I2C_BURST, B_CTRL, B_SD_WRITE, B_SD_READ, B_OLED_TXN, B_UART_LINE, B_LONG,
  B_BUS_WAIT,                         /* ЛР4: очікування м'ютекса I²C задачею imu */
  T_COUNT
};
enum { L_BTN_DB = 0, L_BTN_EDGE, L_CMD, L_CTRL, L_COUNT };

typedef struct { uint32_t t_ms; int16_t pan, tilt; } rec_item_t;   /* imu → journal */
enum { LONG_DUMP = 1, LONG_CALIB = 2 };

/* I²C під м'ютексом: кожна транзакція — окремий захват (imu чекає не весь кадр OLED,
   а лише поточну транзакцію) */
typedef struct {
  const bus_i2c_t *inner;
  os_mutex_t mx;
  struct app *app;
} locked_i2c_t;

typedef struct app {
  const bus_i2c_t *i2c;               /* під м'ютексом (locked) */
  const bus_i2c_t *i2c_raw;
  const bus_spi_t *sd_spi;
  bus_i2c_t  i2c_locked;
  locked_i2c_t i2c_lk;
  mpu6050_t imu;
  ssd1306_t oled;
  sdcard_t  sd;
  bool imu_ok, oled_ok, sd_ok;

  volatile app_mode_t mode;
  attitude_t att;
  servo_axis_t pan, tilt;
  int32_t  dz_cd;
  volatile int32_t pitch_cd, roll_cd;
  bool     ovr;
  int32_t  ovr_pitch, ovr_roll;
  int      imu_status;
  uint32_t imu_errors, imu_samples;

  uint32_t imu_period_us, imu_next, imu_prev;
  uint32_t t_pot, t_oled, t_telem, t_rec;
  uint16_t telem_ms;
  uint8_t  oled_mode, oled_step;
  uint32_t oled_frames, oled_frames_t0;
  bool     evt;
  uint32_t busy_us;
  uint16_t rec_div;

  /* журнал (під jr_mx) */
  traj_enc_t enc;
  traj_dir_t dir;
  bool       dir_ok;
  uint32_t rec_t0, rec_prev, rec_records, rec_gaps, rec_max_int, rec_drops;
  uint16_t rec_blocks;
  uint16_t play_blk, play_i, play_n;
  int16_t  play_pan, play_tilt;
  uint32_t play_t0, play_records;
  bool     play_done;
  uint8_t  blk[TRAJ_BLOCK];
  uint8_t  dump_blk[TRAJ_BLOCK];      /* ЛР4: окремий буфер dump — обмеження 5 ЛР3 */

  /* ОС */
  os_mutex_t jr_mx;                   /* журнал + SD + режим */
  os_queue_t q_rec, q_long;
  os_task_t  t_btn, t_imu, t_journal, t_cmd, tk_telem, tk_oled, t_long;

  /* вимірювання */
  perf_stat_t st[T_COUNT];
  perf_stat_t lat[L_COUNT];
  perf_jit_t  jit_imu;
  perf_load_t load;
  uint32_t loop_prev;
  uint32_t first_imu_us;
  uint32_t load_win_t0, load_idle0;
} app_t;

void app_init(app_t *a, const bus_i2c_t *i2c, const bus_spi_t *sd_spi);
void app_os_objects(app_t *a);                 /* м'ютекси і черги (див. коментар в app.c) */
void app_start(app_t *a, bool adc_ok);         /* банер, FR-12, каталог журналу (FR-09) */
void app_handle_line(app_t *a, char *line);    /* одна команда консолі */
void app_set_mode(app_t *a, app_mode_t m);
void app_stats(app_t *a);
void app_stats_reset(app_t *a);
const char *app_mode_str(app_mode_t m);

/* Кроки задач (одна «робота» кожна) — спільні для RTOS і кооперативного режиму */
void app_imu_run(app_t *a);
void app_pot_run(app_t *a);
void app_btn_run(app_t *a);
void app_rec_drain(app_t *a, uint32_t wait_ms);
void app_play_run(app_t *a);
void app_oled_run(app_t *a);
void app_telem_run(app_t *a);
void app_cmd_run(app_t *a);
void app_long_run(app_t *a, int op);
bool app_btn_pending(void);
void app_set_btn_task(os_task_t t);

#ifdef APP_COOP
void app_poll(app_t *a);                       /* кооперативний планувальник для хостових тестів */
#endif

/* RTOS (App/app_rtos.c): створити об'єкти ядра і задачі, запустити планувальник */
void app_rtos_start(app_t *a) __attribute__((__noreturn__));
/* Команди ОС (`tasks`, `trace`, `os …`): true — команду оброблено */
bool app_os_cmd(app_t *a, const char *cmd, char *arg);

/* SysTick tick hook: debounce кнопки MODE (FR-06) */
void     app_tick_1ms(void);
uint32_t app_btn_presses(void);
int      app_btn_stable_level(void);

#endif
