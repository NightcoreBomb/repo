/**
 * app.c — логіка продукту (ЛР3 → ЛР4): кроки задач, режими, консоль, вимірювання.
 * ЛР4: кроки викликаються задачами FreeRTOS (App/app_rtos.c); спільні дані — під м'ютексами
 * (журнал/SD/режим — jr_mx, I²C — на рівні транзакції), передача imu → journal — чергою.
 */
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "selftest.h"
#include "plat.h"
#include "servo.h"
#include "pot.h"

#ifndef FW_ID
#define FW_ID "dev"
#endif

/* ================= кнопка (переривання SysTick) ================= */
static button_t btn;
static volatile int btn_ready;
static volatile uint32_t btn_pending, btn_press_cyc, btn_edge_cyc;
static os_task_t volatile btn_task;           /* ЛР4: кого будити (NULL до старту ОС) */

bool app_btn_pending(void) { return btn_pending != 0; }
void app_set_btn_task(os_task_t t) { btn_task = t; }

void app_tick_1ms(void)
{
  if (!btn_ready) return;
  if (button_update(&btn, plat_btn_level(), plat_millis()) == BTN_EV_PRESS) {
    btn_press_cyc = plat_cycles();         /* debounce підтвердив натискання */
    /* ЛР3 П-19: мітку першого фронту фіксуємо тут, поки кнопку ще тримають. Якщо цикл
     * зайнятий довше за утримання, брязкіт відпускання (Wokwi зливає rise+fall в одне
     * переривання) перезаписував мітку — затримка виходила заниженою або «від'ємною». */
    btn_edge_cyc = plat_btn_first_edge_cyc();
    btn_pending++;
    os_notify_isr(btn_task);               /* ЛР4: tick hook (ISR) лише будить задачу btn */
  }
}

uint32_t app_btn_presses(void)      { return btn.presses; }
int      app_btn_stable_level(void) { return btn.stable; }

/* ================= допоміжне ================= */
static const char *const st_name[T_COUNT] = {
  "loop", "cmd", "btn", "imu", "pot", "rec", "play", "oled", "telem",
  "i2c_burst", "ctrl", "sd_write", "sd_read", "oled_txn", "uart_line", "long_op", "bus_wait"
};
static const char *const lat_name[L_COUNT] = { "btn_db", "btn_edge", "cmd", "imu2pwm" };

static uint32_t cyc_us(uint32_t cyc) { return cyc / plat_cyc_per_us(); }

/* Кінець виміряного відрізка: STAT + (для задач, що виконали роботу) зайнятість CPU */
static void done(app_t *a, int id, uint32_t t0)
{
  uint32_t d = plat_cycles() - t0;
  perf_add_us(&a->st[id], cyc_us(d));
#ifdef APP_COOP
  if (id < B_I2C_BURST) perf_load_busy(&a->load, d);
#endif
}

static void evt(app_t *a, const char *name)
{
  if (a->evt) plat_log("EVT t_us=%lu name=%s\r\n", (unsigned long)plat_us(), name);
}

const char *app_mode_str(app_mode_t m)
{
  return m == MODE_REC ? "REC" : (m == MODE_PLAY ? "PLAY" : "LIVE");
}

static void servo_apply(app_t *a, uint16_t pan_us, uint16_t tilt_us)
{
  plat_probe(PROBE_PWM, true);
  plat_servo_set_us(0, pan_us);
  plat_servo_set_us(1, tilt_us);
  plat_probe(PROBE_PWM, false);
  (void)a;
}

/* SD — лише під jr_mx (викликач тримає м'ютекс) */
static int sd_write(app_t *a, uint32_t lba)
{
  plat_probe(PROBE_LONG, true);
  uint32_t t0 = plat_cycles();
  int s = sd_write_block(&a->sd, lba, a->blk);
  done(a, B_SD_WRITE, t0);
  plat_probe(PROBE_LONG, false);
  return s;
}

static int sd_read_to(app_t *a, uint32_t lba, uint8_t *buf)
{
  plat_probe(PROBE_LONG, true);
  uint32_t t0 = plat_cycles();
  int s = sd_read_block(&a->sd, lba, buf);
  done(a, B_SD_READ, t0);
  plat_probe(PROBE_LONG, false);
  return s;
}

static int sd_read(app_t *a, uint32_t lba) { return sd_read_to(a, lba, a->blk); }

/* ================= I²C під м'ютексом ================= */
static int lk_take(locked_i2c_t *l)
{
  uint32_t t0 = plat_cycles();
  /* ЛР4 П-27: без тайм-ауту. Час утримання шини обмежений однією транзакцією (≤ 15 мс),
     а успадкування пріоритету не дає задачі нижчого пріоритету затримати власника. Тайм-аут
     лише додавав рідкісний шлях vTaskPriorityDisinheritAfterTimeout. */
  bool ok = os_lock(l->mx, OS_FOREVER);
  if (os_task_self() == l->app->t_imu) done(l->app, B_BUS_WAIT, t0);   /* скільки IMU чекав шину */
  return ok ? ST_OK : ST_EBUSY;
}
static int lk_write(void *c, uint8_t ad, const uint8_t *d, size_t n)
{
  locked_i2c_t *l = c; int s = lk_take(l); if (s) return s;
  s = l->inner->write(l->inner->ctx, ad, d, n); os_unlock(l->mx); return s;
}
static int lk_read(void *c, uint8_t ad, uint8_t *d, size_t n)
{
  locked_i2c_t *l = c; int s = lk_take(l); if (s) return s;
  s = l->inner->read(l->inner->ctx, ad, d, n); os_unlock(l->mx); return s;
}
static int lk_wr(void *c, uint8_t ad, const uint8_t *w, size_t nw, uint8_t *r, size_t nr)
{
  locked_i2c_t *l = c; int s = lk_take(l); if (s) return s;
  s = l->inner->write_read(l->inner->ctx, ad, w, nw, r, nr); os_unlock(l->mx); return s;
}
static int lk_probe(void *c, uint8_t ad)
{
  locked_i2c_t *l = c; int s = lk_take(l); if (s) return s;
  s = l->inner->probe(l->inner->ctx, ad); os_unlock(l->mx); return s;
}

/* ================= ініціалізація ================= */
void app_stats_reset(app_t *a)
{
  for (int i = 0; i < T_COUNT; i++) { perf_reset(&a->st[i]); a->st[i].name = st_name[i]; }
  for (int i = 0; i < L_COUNT; i++) { perf_reset(&a->lat[i]); a->lat[i].name = lat_name[i]; }
  a->jit_imu.name = "imu";
  a->jit_imu.period_us = a->imu_period_us;
  perf_jit_reset(&a->jit_imu);
  perf_load_reset(&a->load, plat_cycles(), 1000u * 1000u * plat_cyc_per_us());
  a->oled_frames = 0;
  a->oled_frames_t0 = plat_millis();
  a->rec_gaps = a->rec_max_int = a->rec_drops = 0;
  a->loop_prev = 0;
  if (os_running()) os_stats_reset();
  a->load_win_t0 = plat_us();
  a->load_idle0 = os_running() ? os_idle_us() : 0;
}

/* Об'єкти ядра. На STM32 — у app_rtos_start, ПІСЛЯ app_start: перша ж критична секція FreeRTOS
   до старту планувальника залишає переривання вимкненими (uxCriticalNesting = 0xAAAAAAAA), а
   app_start ще чекає пристрої через HAL_Delay → SysTick стоїть → зависання (журнал ЛР4, П-22). */
void app_os_objects(app_t *a)
{
  a->i2c_lk.mx = os_mutex_new(true);
  a->jr_mx = os_mutex_new(true);
  a->q_rec = os_queue_new(8, sizeof(rec_item_t));
  a->q_long = os_queue_new(4, sizeof(int));
  if (!a->jr_mx || !a->i2c_lk.mx || !a->q_rec || !a->q_long) plat_log("FATAL os objects (heap)\r\n");
}

void app_init(app_t *a, const bus_i2c_t *i2c, const bus_spi_t *sd_spi)
{
  btn_ready = 0;
  button_init(&btn, plat_btn_level(), plat_millis(), BUTTON_DEBOUNCE_MS);
  btn_pending = 0;
  btn_ready = 1;
  memset(a, 0, sizeof *a);
  a->i2c_raw = i2c;
  a->i2c_lk.inner = i2c;
  a->i2c_lk.app = a;
  a->i2c_locked = (bus_i2c_t){ &a->i2c_lk, lk_write, lk_read, lk_wr, lk_probe };
  a->i2c = &a->i2c_locked;
  a->sd_spi = sd_spi;
#ifdef APP_COOP
  app_os_objects(a);
#endif
  mpu6050_setup(&a->imu, a->i2c, MPU6050_ADDR_AD0_LOW);
  ssd1306_setup(&a->oled, a->i2c, SSD1306_ADDR_SA0_LOW);
  sd_setup(&a->sd, sd_spi);
  att_reset(&a->att);
  a->pan.out_cd = a->tilt.out_cd = 9000;
  a->imu_period_us = IMU_PERIOD_US;
  a->telem_ms = TELEM_PERIOD_MS;
  a->evt = true;
  plat_servo_set_us(0, SERVO_US_CENTER);   /* FR-12: серво в центр до будь-яких перевірок */
  plat_servo_set_us(1, SERVO_US_CENTER);
  app_stats_reset(a);
}

static void load_dir(app_t *a)
{
  a->dir_ok = a->sd_ok && sd_read(a, TRAJ_DIR_LBA) == ST_OK && traj_dir_read(a->blk, &a->dir);
  if (!a->dir_ok) memset(&a->dir, 0, sizeof a->dir);
}

void app_start(app_t *a, bool adc_ok)
{
  evt(a, "boot");
  plat_log("\r\n==== %s FreeRTOS | fw=%s | %s | SYSCLK=%lu | tick=%lu | reset=0x%02lX ====\r\n",
           APP_FW_TAG, FW_ID, plat_board_name(), (unsigned long)plat_sysclk_hz(),
           (unsigned long)os_tick_hz(), (unsigned long)plat_reset_cause());

  /* FR-12: MPU-6050 (WHO_AM_I = 0x68) і SD — перевірка при старті */
  int s = mpu6050_identify(&a->imu);
  if (s == ST_OK) s = mpu6050_init(&a->imu);
  a->imu_ok = (s == ST_OK);
  a->imu_status = s;
  int so = ssd1306_init(&a->oled);
  if (so == ST_OK) so = ssd1306_clear(&a->oled);
  a->oled_ok = (so == ST_OK);
  int ss = sd_init(&a->sd);
  a->sd_ok = (ss == ST_OK);
  load_dir(a);

  plat_log("CHK imu=%s who=0x%02X oled=%s sd=%s type=%s adc=%s\r\n",
           a->imu_ok ? "OK" : st_str(s), a->imu.who_am_i, a->oled_ok ? "OK" : st_str(so),
           a->sd_ok ? "OK" : st_str(ss), sd_type_str(a->sd.type), adc_ok ? "OK" : "FAIL");
  if (!a->imu_ok) plat_log("ERR IMU not found: servo centered, LIVE disabled\r\n");
  if (!a->sd_ok)  plat_log("ERR SD not ready: REC/PLAY disabled\r\n");
  plat_log("DIR %s blocks=%u records=%lu duration_ms=%lu\r\n", a->dir_ok ? "OK" : "NONE",
           a->dir.blocks, (unsigned long)a->dir.records, (unsigned long)a->dir.duration_ms);
  if (a->oled_ok) {
    ssd1306_line(&a->oled, 0, "MOTION MIRROR LR4");
    ssd1306_line(&a->oled, 1, a->imu_ok ? "IMU OK" : "IMU NOT FOUND");
    ssd1306_line(&a->oled, 2, a->sd_ok ? "SD OK" : "SD ERROR");
  }
  /* dead-zone — ДО першої ітерації циклу: інакше перший запуск imu_task бачить dz = 0 і
     смикає серво на крок 1.8° (знайдено тестом test_deadzone_from_pot_fr04, журнал П-15) */
  int raw = plat_pot_read_raw();
  if (raw >= 0) a->dz_cd = pot_raw_to_deadzone_cdeg((uint32_t)raw);
  uint32_t now = plat_millis();
  a->imu_next = plat_cycles();
  a->t_pot = now + POT_PERIOD_MS;
  a->t_oled = a->t_telem = now;
  app_stats_reset(a);
  plat_log("READY (type 'help')\r\n");
}

/* ================= режими (FR-06..FR-09) ================= */
static void rec_flush(app_t *a)
{
  if (a->enc.n == 0) return;
  traj_enc_finish(&a->enc);
  if (sd_write(a, TRAJ_DIR_LBA + 1u + a->rec_blocks) == ST_OK) a->rec_blocks++;
  else plat_log("ERR sd_write blk=%u\r\n", a->rec_blocks);
  a->enc.n = 0;
}

static void rec_add(app_t *a, const rec_item_t *r);

static void rec_stop(app_t *a)
{
  rec_item_t r;
  while (os_queue_recv(a->q_rec, &r, 0)) rec_add(a, &r);   /* дописати те, що ще в черзі */
  rec_flush(a);
  a->dir.blocks = a->rec_blocks;
  a->dir.records = a->rec_records;
  a->dir.duration_ms = a->rec_records * REC_PERIOD_MS;
  a->dir.period_ms = REC_PERIOD_MS;
  traj_dir_write(a->blk, &a->dir);
  a->dir_ok = (sd_write(a, TRAJ_DIR_LBA) == ST_OK);
  plat_log("REC stop records=%lu blocks=%u duration_ms=%lu gaps=%lu max_int_ms=%lu\r\n",
           (unsigned long)a->rec_records, a->rec_blocks, (unsigned long)a->dir.duration_ms,
           (unsigned long)a->rec_gaps, (unsigned long)a->rec_max_int);
  evt(a, "rec_stop");
}

static bool play_load(app_t *a)
{
  if (a->play_blk >= a->dir.blocks) return false;
  if (sd_read(a, TRAJ_DIR_LBA + 1u + a->play_blk) != ST_OK) return false;
  int n = traj_block_check(a->blk, a->play_blk);
  if (n < 0) { plat_log("ERR play blk=%u crc\r\n", a->play_blk); return false; }
  a->play_n = (uint16_t)n;
  a->play_i = 0;
  return true;
}

void app_set_mode(app_t *a, app_mode_t m)
{
  if (m == a->mode) return;
  if ((m == MODE_REC || m == MODE_PLAY) && !a->sd_ok) {
    plat_log("ERR SD not ready, mode stays %s\r\n", app_mode_str(a->mode));
    return;
  }
  os_lock(a->jr_mx, OS_FOREVER);
  if (a->mode == MODE_REC) rec_stop(a);
  a->mode = m;
  uint32_t now = plat_millis();
  if (m == MODE_REC) {
    rec_item_t junk;
    while (os_queue_recv(a->q_rec, &junk, 0)) { }
    a->rec_records = a->rec_blocks = 0;
    a->rec_gaps = a->rec_max_int = a->rec_drops = 0;
    a->rec_div = 0;
    a->rec_t0 = a->t_rec = now;
    traj_enc_begin(&a->enc, a->blk, 0, 0);
    evt(a, "rec_start");
  } else if (m == MODE_PLAY) {
    a->play_blk = 0;
    a->play_records = 0;
    a->play_done = !a->dir_ok || !play_load(a);
    a->play_t0 = a->t_rec = now;
    if (a->play_done) plat_log("ERR no recording\r\n");
    evt(a, "play_start");
  } else {
    evt(a, "live");
  }
  plat_log("MODE %s\r\n", app_mode_str(m));
  os_unlock(a->jr_mx);
  if (m == MODE_PLAY) os_notify(a->t_journal);         /* journal: перейти до відтворення */
}

/* ================= задачі ================= */
void app_btn_run(app_t *a)
{
  if (!btn_pending) return;
  uint32_t t0 = plat_cycles();
  plat_mark(true);                                     /* реакція на кнопку почалась */
  btn_pending = 0;
  uint32_t lat_db = cyc_us(t0 - btn_press_cyc), lat_edge = cyc_us(t0 - btn_edge_cyc);
  perf_add_us(&a->lat[L_BTN_DB], lat_db);
  perf_add_us(&a->lat[L_BTN_EDGE], lat_edge);
  app_set_mode(a, a->mode == MODE_LIVE ? MODE_REC : (a->mode == MODE_REC ? MODE_PLAY : MODE_LIVE));
  plat_mark(false);
  if (a->evt) plat_log("EVT t_us=%lu name=btn lat_db_us=%lu lat_edge_us=%lu\r\n",
                       (unsigned long)plat_us(), (unsigned long)lat_db, (unsigned long)lat_edge);
  done(a, T_BTN, t0);
}

void app_imu_run(app_t *a)
{
  uint32_t now = plat_cycles();
  plat_probe(PROBE_IMU, true);
  perf_jit_tick(&a->jit_imu, now, plat_cyc_per_us());

  if (a->imu_ok) {
    mpu6050_raw_t r;
    mpu6050_data_t d;
    uint32_t t1 = plat_cycles();
    int s = mpu6050_read_raw(&a->imu, &r);
    done(a, B_I2C_BURST, t1);
    if (s == ST_OK) {
      mpu6050_convert(&a->imu, &r, &d);
      att_update(&a->att, &d, a->imu_samples ? cyc_us(now - a->imu_prev) : 0);
      a->imu_prev = now;
      if (!a->imu_samples++) a->first_imu_us = plat_us();
      if (a->imu_status != ST_OK) evt(a, "imu_ok");
    } else {
      a->imu_errors++;
      if (a->imu_status == ST_OK) evt(a, "imu_err");
    }
    a->imu_status = s;
  }
  a->pitch_cd = a->ovr ? a->ovr_pitch : att_pitch_cd(&a->att);
  a->roll_cd  = a->ovr ? a->ovr_roll  : att_roll_cd(&a->att);

  /* керування (FR-03..05): у LIVE і REC серво повторюють руку; у PLAY — журнал */
  if (a->mode != MODE_PLAY && (a->imu_ok || a->ovr) && a->imu_status == ST_OK) {
    uint32_t t2 = plat_cycles();
    bool ch = ctrl_axis_update(&a->pan, 9000 + a->roll_cd, a->dz_cd, a->imu_period_us);
    ch |= ctrl_axis_update(&a->tilt, 9000 + a->pitch_cd, a->dz_cd, a->imu_period_us);
    if (ch) {
      servo_apply(a, servo_cdeg_to_us(a->pan.out_cd), servo_cdeg_to_us(a->tilt.out_cd));
      perf_add_us(&a->lat[L_CTRL], cyc_us(plat_cycles() - now));   /* початок задачі → новий CCR */
    }
    done(a, B_CTRL, t2);
  }
  /* REC (FR-07): кожен (20 мс / період IMU)-й відлік — у чергу журналу. Відмітка часу —
     момент відліку, тож інтервали між записами не залежать від затримок SD. */
  if (a->mode == MODE_REC) {
    uint32_t div = (REC_PERIOD_MS * 1000u) / a->imu_period_us;
    if (++a->rec_div >= (div ? div : 1u)) {
      a->rec_div = 0;
      rec_item_t r = { plat_millis(), (int16_t)(a->pan.out_cd / 10), (int16_t)(a->tilt.out_cd / 10) };
      if (!os_queue_send(a->q_rec, &r, 0)) a->rec_drops++;   /* journal не встигає — не блокуємо IMU */
    }
  }
  plat_probe(PROBE_IMU, false);
  done(a, T_IMU, now);
}

#ifdef APP_COOP
static void task_imu(app_t *a)
{
  uint32_t now = plat_cycles(), per = a->imu_period_us * plat_cyc_per_us();
  if ((int32_t)(now - a->imu_next) < 0) return;
  a->imu_next += per;
  if ((int32_t)(now - a->imu_next) >= 0) a->imu_next = now + per;
  app_imu_run(a);
}
#endif

void app_pot_run(app_t *a)
{
  uint32_t t0 = plat_cycles();
  int raw = plat_pot_read_raw();
  if (raw >= 0) a->dz_cd = pot_raw_to_deadzone_cdeg((uint32_t)raw);
  done(a, T_POT, t0);
}

#ifdef APP_COOP
static void task_pot(app_t *a)
{
  uint32_t ms = plat_millis();
  if ((int32_t)(ms - a->t_pot) < 0) return;
  a->t_pot += POT_PERIOD_MS;
  if ((int32_t)(ms - a->t_pot) >= 0) a->t_pot = ms + POT_PERIOD_MS;
  app_pot_run(a);
}
#endif

static void rec_add(app_t *a, const rec_item_t *r)
{
  uint32_t t0 = plat_cycles();
  if (a->rec_records) {
    uint32_t dt = r->t_ms - a->rec_prev;
    if (dt > REC_GAP_MS) a->rec_gaps++;
    if (dt > a->rec_max_int) a->rec_max_int = dt;
  }
  a->rec_prev = r->t_ms;
  a->rec_records++;
  if (traj_enc_add(&a->enc, r->pan, r->tilt)) {
    rec_flush(a);
    if (a->rec_blocks >= TRAJ_MAX_BLOCKS) plat_log("REC full\r\n");
    else traj_enc_begin(&a->enc, a->blk, a->rec_blocks, a->rec_records * REC_PERIOD_MS);
  }
  done(a, T_REC, t0);
}

/* journal (REC): забрати з черги все, що є (перший — з очікуванням wait_ms) */
void app_rec_drain(app_t *a, uint32_t wait_ms)
{
  rec_item_t r;
  while (os_queue_recv(a->q_rec, &r, wait_ms)) {
    wait_ms = 0;
    os_lock(a->jr_mx, OS_FOREVER);
    if (a->mode == MODE_REC && a->rec_blocks < TRAJ_MAX_BLOCKS) rec_add(a, &r);
    os_unlock(a->jr_mx);
  }
}

void app_play_run(app_t *a)
{
  uint32_t ms = plat_millis();
  os_lock(a->jr_mx, OS_FOREVER);
  if (a->mode != MODE_PLAY || a->play_done) { os_unlock(a->jr_mx); return; }
  uint32_t t0 = plat_cycles();
  if (a->play_i >= a->play_n) {
    a->play_blk++;
    if (!play_load(a)) {
      a->play_done = true;
      plat_log("PLAY done records=%lu duration_ms=%lu rec_ms=%lu\r\n",
               (unsigned long)a->play_records, (unsigned long)(ms - a->play_t0),
               (unsigned long)a->dir.duration_ms);
      evt(a, "play_done");
      done(a, T_PLAY, t0);
      os_unlock(a->jr_mx);
      return;
    }
  }
  traj_block_step(a->blk, a->play_i++, &a->play_pan, &a->play_tilt);
  a->play_records++;
  a->pan.out_cd = a->play_pan * 10;
  a->tilt.out_cd = a->play_tilt * 10;
  servo_apply(a, servo_cdeg_to_us(a->pan.out_cd), servo_cdeg_to_us(a->tilt.out_cd));
  done(a, T_PLAY, t0);
  os_unlock(a->jr_mx);
}

#ifdef APP_COOP
static void task_rec(app_t *a)  { if (a->mode == MODE_REC) app_rec_drain(a, 0); }
static void task_play(app_t *a)
{
  uint32_t ms = plat_millis();
  if (a->mode != MODE_PLAY || a->play_done || (int32_t)(ms - a->t_rec) < 0) return;
  a->t_rec += REC_PERIOD_MS;
  app_play_run(a);
}
#endif

/* OLED (FR-10): 4 рядки по 21 символу. Режим 0 — усе блокувально за раз (super loop
   «як є»); 1 — один рядок за виклик; 2 — одна I²C-транзакція за виклик (машина станів). */
#define OLED_LINES 4u
#define OLED_PARTS 5u                    /* вікно + 4 шматки даних по 32 Б */

static void oled_text(app_t *a, unsigned line, char *s, size_t n)
{
  int32_t p = a->pitch_cd, r = a->roll_cd;
  switch (line) {
    case 0: plat_fmt(s, n, "%-4s DZ %ld.%ld %s", app_mode_str(a->mode), (long)(a->dz_cd / 100),
                     (long)(a->dz_cd % 100 / 10), a->imu_status == ST_OK ? "" : "IMU!"); break;
    case 1: plat_fmt(s, n, "P%c%ld.%02ld R%c%ld.%02ld", p < 0 ? '-' : '+', (long)(abs(p) / 100),
                     (long)(abs(p) % 100), r < 0 ? '-' : '+', (long)(abs(r) / 100), (long)(abs(r) % 100)); break;
    case 2: plat_fmt(s, n, "PAN %u TLT %u", plat_servo_get_us(0), plat_servo_get_us(1)); break;
    default:
      if (a->mode == MODE_REC) plat_fmt(s, n, "REC %lu S", (unsigned long)(a->rec_records / 50u));
      else if (a->mode == MODE_PLAY) plat_fmt(s, n, "PLAY %lu/%lu", (unsigned long)(a->play_records / 50u),
                                              (unsigned long)(a->dir.duration_ms / 1000u));
      else plat_fmt(s, n, "T %lu S", (unsigned long)(plat_millis() / 1000u));
  }
}

static void oled_part(app_t *a, unsigned line, unsigned part, uint8_t *buf)
{
  uint32_t t0 = plat_cycles();
  if (part == 0) {
    char s[24];
    oled_text(a, line, s, sizeof s);
    ssd1306_render_line(s, buf);
    ssd1306_window(&a->oled, (uint8_t)line, 0);
  } else {
    size_t off = (part - 1u) * 32u, k = SSD1306_LINE_BYTES - off < 32u ? SSD1306_LINE_BYTES - off : 32u;
    ssd1306_write_data(&a->oled, buf + off, k);
  }
  done(a, B_OLED_TXN, t0);
}

void app_oled_run(app_t *a)
{
  static uint8_t buf[SSD1306_LINE_BYTES];
  if (!a->oled_ok) return;
  uint32_t t0 = plat_cycles();
  unsigned per_call = a->oled_mode == 0 ? OLED_LINES * OLED_PARTS : (a->oled_mode == 1 ? OLED_PARTS : 1u);
  for (unsigned i = 0; i < per_call; i++) {
    unsigned st = a->oled_step;
    oled_part(a, st / OLED_PARTS, st % OLED_PARTS, buf);
    a->oled_step = (uint8_t)((st + 1u) % (OLED_LINES * OLED_PARTS));
    if (a->oled_step == 0) a->oled_frames++;
  }
  done(a, T_OLED, t0);
}

#ifdef APP_COOP
static void task_oled(app_t *a)
{
  uint32_t ms = plat_millis();
  if (!a->oled_ok || (int32_t)(ms - a->t_oled) < 0) return;
  uint32_t steps = a->oled_mode == 0 ? 1u : (a->oled_mode == 1 ? OLED_LINES : OLED_LINES * OLED_PARTS);
  a->t_oled += OLED_PERIOD_MS / steps;
  if ((int32_t)(ms - a->t_oled) >= 0) a->t_oled = ms + OLED_PERIOD_MS / steps;
  app_oled_run(a);
}
#endif

/* Телеметрія FR-11: t,pitch,roll,pan,tilt,mode (мс, 0.01°, мкс) — блокувальний UART */
void app_telem_run(app_t *a)
{
  uint32_t ms = plat_millis();
  uint32_t t0 = plat_cycles();
  plat_log("%lu,%ld,%ld,%u,%u,%s\r\n", (unsigned long)ms, (long)a->pitch_cd, (long)a->roll_cd,
           plat_servo_get_us(0), plat_servo_get_us(1), app_mode_str(a->mode));
  done(a, B_UART_LINE, t0);
  done(a, T_TELEM, t0);
}

#ifdef APP_COOP
static void task_telem(app_t *a)
{
  uint32_t ms = plat_millis();
  if (!a->telem_ms || (int32_t)(ms - a->t_telem) < 0) return;
  a->t_telem += a->telem_ms;
  if ((int32_t)(ms - a->t_telem) >= 0) a->t_telem = ms + a->telem_ms;
  app_telem_run(a);
}
#endif

/* ================= довгі операції (команди) ================= */
static void long_begin(app_t *a, const char *name)
{
  plat_probe(PROBE_LONG, true);
  if (a->evt) plat_log("EVT t_us=%lu name=%s_start\r\n", (unsigned long)plat_us(), name);
}

static void long_end(app_t *a, const char *name, uint32_t t0)
{
  done(a, B_LONG, t0);
  plat_probe(PROBE_LONG, false);
  if (a->evt) plat_log("EVT t_us=%lu name=%s_done dur_us=%lu\r\n", (unsigned long)plat_us(), name,
                       (unsigned long)cyc_us(plat_cycles() - t0));
}

/* dump: увесь журнал по UART у hex — «передача даних на хост» (секунди) */
/* ЛР4: власний буфер (a->dump_blk) — dump більше не ділить буфер з записом/відтворенням
   (обмеження 5 ЛР3, П-17/П-18), тож дозволений і в REC; SD — під jr_mx лише на час читання
   блоку, друк рядків — поза м'ютексом журналу. */
static void cmd_dump(app_t *a)
{
  uint32_t t0 = plat_cycles();
  long_begin(a, "dump");
  os_lock(a->jr_mx, OS_FOREVER);
  traj_dir_t dir = a->dir;
  bool ok = a->dir_ok;
  os_unlock(a->jr_mx);
  plat_log("DUMP blocks=%u records=%lu\r\n", dir.blocks, (unsigned long)dir.records);
  for (uint16_t b = 0; ok && b < dir.blocks; b++) {
    os_lock(a->jr_mx, OS_FOREVER);
    int s = sd_read_to(a, TRAJ_DIR_LBA + 1u + b, a->dump_blk);
    os_unlock(a->jr_mx);
    if (s != ST_OK) break;
    for (unsigned off = 0; off < TRAJ_BLOCK; off += 32u) {
      char line[80];
      size_t n = plat_fmt(line, sizeof line, "D %u %03u ", b, off);
      for (unsigned i = 0; i < 32u; i++) n += plat_fmt(line + n, sizeof line - n, "%02X", a->dump_blk[off + i]);
      plat_log("%s\r\n", line);
    }
  }
  plat_log("DUMP end\r\n");
  long_end(a, "dump", t0);
}

/* calib: середній зсув нуля гіроскопа за 200 відліків (≈ 1 с, блокувально) */
static void cmd_calib(app_t *a)
{
  int32_t s[3] = { 0, 0, 0 };
  int n = 0;
  uint32_t t0 = plat_cycles();
  long_begin(a, "calib");
  for (int i = 0; a->imu_ok && i < 200; i++) {
    mpu6050_raw_t r;
    mpu6050_data_t d;
    if (mpu6050_read_raw(&a->imu, &r) == ST_OK) {
      mpu6050_convert(&a->imu, &r, &d);
      s[0] += d.gx_mdps; s[1] += d.gy_mdps; s[2] += d.gz_mdps;
      n++;
    }
    plat_delay_ms(5);
  }
  if (n) for (int k = 0; k < 3; k++) a->att.bias_mdps[k] = s[k] / n;
  long_end(a, "calib", t0);
  plat_log("CALIB n=%d bias_mdps=%ld,%ld,%ld\r\n", n, (long)a->att.bias_mdps[0],
           (long)a->att.bias_mdps[1], (long)a->att.bias_mdps[2]);
}

/* long_op: виконати довгу операцію (у RTOS — задача з найнижчим робочим пріоритетом) */
void app_long_run(app_t *a, int op)
{
  if (op == LONG_DUMP) cmd_dump(a);
  else if (op == LONG_CALIB) { cmd_calib(a); plat_log("OK calib\r\n"); }
}

/* ================= статистика ================= */
static void print_stat(const char *kind, const perf_stat_t *s)
{
  if (!s->n) return;
  plat_log("%s name=%s n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n", kind, s->name,
           (unsigned long)s->n, (unsigned long)s->min_us, (unsigned long)perf_avg_us(s),
           (unsigned long)s->max_us);
}

void app_stats(app_t *a)
{
  uint32_t ms = plat_millis(), dt = ms - a->oled_frames_t0;
  plat_log("STAT BEGIN fw=%s t_ms=%lu mode=%s oled_mode=%u telem_ms=%u imu_hz=%lu load_us=%lu\r\n",
           FW_ID, (unsigned long)ms, app_mode_str(a->mode), a->oled_mode, a->telem_ms,
           (unsigned long)(1000000u / a->imu_period_us), (unsigned long)a->busy_us);
  for (int i = 0; i < T_COUNT; i++) print_stat("STAT", &a->st[i]);
  for (int i = 0; i < L_COUNT; i++) print_stat("LAT", &a->lat[i]);
  const perf_jit_t *j = &a->jit_imu;
  plat_log("JIT name=imu period_us=%lu n=%lu min_us=%ld max_us=%ld p99_us=%lu missed=%lu\r\n",
           (unsigned long)j->period_us, (unsigned long)j->n, (long)(j->n ? j->jmin_us : 0),
           (long)(j->n ? j->jmax_us : 0), (unsigned long)perf_jit_p99_us(j), (unsigned long)j->missed);
  plat_log("LOAD last_pct=%u min_pct=%u max_pct=%u windows=%lu\r\n", a->load.last_pct,
           a->load.windows ? a->load.min_pct : 0, a->load.max_pct, (unsigned long)a->load.windows);
  plat_log("RX bytes=%lu dropped=%lu ore=%lu\r\n", (unsigned long)plat_console_rx_bytes(),
           (unsigned long)plat_console_rx_dropped(), (unsigned long)plat_console_rx_overrun());
  plat_log("I2C recov=%lu timeouts=%lu imu_err=%lu imu_samples=%lu\r\n",
           (unsigned long)plat_i2c_recoveries(), (unsigned long)plat_i2c_timeouts(),
           (unsigned long)a->imu_errors, (unsigned long)a->imu_samples);
  plat_log("REC records=%lu blocks=%u gaps=%lu max_int_ms=%lu\r\n", (unsigned long)a->rec_records,
           a->rec_blocks, (unsigned long)a->rec_gaps, (unsigned long)a->rec_max_int);
  plat_log("OLED frames=%lu fps_x10=%lu\r\n", (unsigned long)a->oled_frames,
           (unsigned long)(dt ? a->oled_frames * 10000u / dt : 0));
  /* ЛР4: stack_used — стек переривань (MSP); heap — використаний максимум heap ОС
     (total − min_ever_free); стеки задач — команда `tasks` */
  plat_log("MEM stack_used=%lu stack_size=%lu ram_static=%lu heap=%lu first_imu_us=%lu\r\n",
           (unsigned long)plat_stack_used(), (unsigned long)plat_stack_size(),
           (unsigned long)plat_ram_static(), (unsigned long)(os_heap_total() - os_heap_min_free()),
           (unsigned long)a->first_imu_us);
  plat_log("QREC drops=%lu\r\n", (unsigned long)a->rec_drops);
  plat_log("STAT END\r\n");
}

static void hist(app_t *a)
{
  plat_log("HIST name=imu");
  for (int i = 0; i < PERF_JH_BINS; i++)
    plat_log(" lt%lu=%u", (unsigned long)perf_jh_edges_us[i], a->jit_imu.hist[i]);
  plat_log("\r\n");
}

/* ================= консоль (FR-11) ================= */
static void help(void)
{
  plat_log("cmds: status mode [live|rec|play] calib stats [reset] hist evt on|off telem <ms>|off "
           "oled 0|1|2 rate <hz> load <us> angle <p_cd> <r_cd>|off fault i2c 0|1 dump selftest "
           "note <txt> reset | LR4: tasks trace start|pins|dump|stop "
           "os inv mutex|sem, os starve <ms>, os qflood <n>, os heap, os overflow, os ctxsw\r\n");
}

/* Ціле зі знаком (замість strtol newlib: −0.6 КБ Flash). *end — куди дочитали. */
static long parse_long(const char *p, const char **end)
{
  while (*p == ' ') p++;
  int neg = (*p == '-');
  if (neg || *p == '+') p++;
  long v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
  if (end) *end = p;
  return neg ? -v : v;
}

void app_handle_line(app_t *a, char *line)
{
  char *arg = strchr(line, ' ');
  if (arg) *arg++ = '\0'; else arg = line + strlen(line);
  long v = parse_long(arg, NULL);

  if (!strcmp(line, "status"))
    plat_log("STATUS mode=%s pitch_cd=%ld roll_cd=%ld pan_us=%u tilt_us=%u dz_cd=%ld imu=%s sd=%d "
             "dir=%u t=%lu\r\n", app_mode_str(a->mode), (long)a->pitch_cd, (long)a->roll_cd,
             plat_servo_get_us(0), plat_servo_get_us(1), (long)a->dz_cd, st_str(a->imu_status),
             a->sd_ok, a->dir.blocks, (unsigned long)plat_millis());
  else if (!strcmp(line, "mode")) {
    if (!strcmp(arg, "live"))      app_set_mode(a, MODE_LIVE);
    else if (!strcmp(arg, "rec"))  app_set_mode(a, MODE_REC);
    else if (!strcmp(arg, "play")) app_set_mode(a, MODE_PLAY);
    else app_set_mode(a, a->mode == MODE_LIVE ? MODE_REC : (a->mode == MODE_REC ? MODE_PLAY : MODE_LIVE));
  }
  else if (!strcmp(line, "calib") || !strcmp(line, "dump")) {
    int op = line[0] == 'c' ? LONG_CALIB : LONG_DUMP;
#ifdef APP_COOP
    app_long_run(a, op);
#else
    if (!os_queue_send(a->q_long, &op, 0)) plat_log("ERR busy: long_op queue full\r\n");
#endif
    return;                                            /* OK calib — після завершення (long_op) */
  }
  else if (!strcmp(line, "stats")) { if (!strcmp(arg, "reset")) { app_stats_reset(a); plat_log("STATS reset\r\n"); } else app_stats(a); }
  else if (!strcmp(line, "hist"))   hist(a);
  else if (!strcmp(line, "evt"))    a->evt = !strcmp(arg, "on");
  else if (!strcmp(line, "telem"))  a->telem_ms = (uint16_t)(v > 0 ? v : 0);
  else if (!strcmp(line, "oled"))   { a->oled_mode = (uint8_t)(v > 2 ? 2 : (v < 0 ? 0 : v)); a->oled_step = 0; }
  else if (!strcmp(line, "rate"))   {
    if (v >= 10 && v <= 2000) {
      /* ЛР4: період IMU — ціле число тіків (vTaskDelayUntil); 400 Гц при 1 кГц тіку → 2 мс */
      uint32_t us = 1000000u / (uint32_t)v, tick_us = 1000000u / os_tick_hz();
      if (os_running()) { us = (us / tick_us) * tick_us; if (!us) us = tick_us; }
      a->imu_period_us = us;
      app_stats_reset(a);
      plat_log("RATE period_us=%lu hz=%lu\r\n", (unsigned long)us, (unsigned long)(1000000u / us));
    }
  }
  else if (!strcmp(line, "load"))   a->busy_us = (uint32_t)(v > 0 ? v : 0);
  else if (!strcmp(line, "angle")) {
    const char *end;
    a->ovr = strcmp(arg, "off") != 0;
    a->ovr_pitch = parse_long(arg, &end);
    a->ovr_roll  = parse_long(end, NULL);
  }
  else if (!strcmp(line, "fault")) { plat_i2c_fault(strstr(arg, "1") != NULL); plat_log("FAULT i2c=%d\r\n", plat_i2c_fault_get()); }
  else if (!strcmp(line, "note"))   { plat_log("NOTE %s\r\n", arg); return; }   /* мітка фази сценарію */
#ifndef LR4_OSTEST
  else if (!strcmp(line, "selftest")) {          /* self-test пише в a->blk і SD — під м'ютексом журналу */
    os_lock(a->jr_mx, OS_FOREVER);
    selftest_run(a);
    os_unlock(a->jr_mx);
  }
#endif
  else if (!strcmp(line, "reset"))  plat_reset();
  else if (!strcmp(line, "help"))   help();
  else if (app_os_cmd(a, line, arg)) return;
  else { plat_log("ERR unknown '%s'\r\n", line); return; }
  if (strcmp(line, "status") && strcmp(line, "stats") && strcmp(line, "dump") && strcmp(line, "hist"))
    plat_log("OK %s\r\n", line);
}

void app_cmd_run(app_t *a)
{
  static char line[40];
  uint32_t t_eol;
  while (plat_console_getline(line, sizeof line, &t_eol)) {
    uint32_t t0 = plat_cycles();
    if (t_eol) perf_add_us(&a->lat[L_CMD], cyc_us(t0 - t_eol));   /* символ кінця рядка → початок обробки */
    app_handle_line(a, line);
    done(a, T_CMD, t0);
  }
}

/* ================= кооперативний режим (хостові тести) ================= */
#ifdef APP_COOP
void app_poll(app_t *a)
{
  uint32_t t = plat_cycles();
  if (a->loop_prev) perf_add_us(&a->st[T_LOOP], cyc_us(t - a->loop_prev));
  a->loop_prev = t;

  app_cmd_run(a);
  app_btn_run(a);
  task_imu(a);
  task_pot(a);
  task_rec(a);
  task_play(a);
  task_oled(a);
  task_telem(a);
  if (a->busy_us) {                                  /* `load N`: штучна робота (точка зламу) */
    uint32_t t0 = plat_cycles(), need = a->busy_us * plat_cyc_per_us();
    while (plat_cycles() - t0 < need) { }
    perf_load_busy(&a->load, need);
  }
  perf_load_poll(&a->load, plat_cycles());
}
#endif
