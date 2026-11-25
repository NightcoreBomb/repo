/**
 * app_rtos.c — організація виконання ЛР4: задачі FreeRTOS, їхні цикли очікування і
 * команди ОС (`tasks`, `trace`, `os …`, сценарії методички §7.2).
 *
 * Логіка кожного кроку — у App/app.c (та сама, що в super loop ЛР3); тут лише «коли»:
 * на чому задача блокується і хто її будить. Схема — docs/rtos-design.md.
 */
#include <string.h>
#include "app.h"
#include "plat.h"
#include "os.h"

/* Розміри стеків, слів (×4 Б). Початкові значення — статична оцінка (-fcallgraph-info=su,
   tools/stack_tree.py) + запас; фінальні — за high water mark після S3 (docs/rtos-design.md §3) */
/*                     статична оцінка (stack_tree.py, з кадром перемикання 64 Б) → слів (≈ +15 %) */
#define STK_BTN      104u   /* 360 Б  */
#define STK_IMU      140u   /* 432 Б → 560 Б: запас ≥ 120 Б за HWM (П-25) */
#define STK_JOURNAL  112u   /* 320 Б → 448 Б: HWM 92 Б < 25 % (web-soak, П-25) */
#define STK_CMD      220u   /* 768 Б (selftest → imu_measure → I²C під м'ютексом) */
#define STK_TELEM    120u   /* 304 Б → 480 Б: HWM у Wokwi показав лише 44 Б запасу (П-25) */
#define STK_OLED     168u   /* 528 Б → 672 Б (П-25) */
#define STK_LONG     156u   /* 488 Б → 624 Б: HWM 124 Б < 25 % (web-soak, П-25) */

#define IMU_RETRY_MS 50u

static uint32_t ms_ticks(uint32_t ms) { return os_ms_to_ticks(ms); }

/* ---------------- задачі продукту ---------------- */

/* btn (6): найжорсткіша вимога до реакції — людина бачить відгук; робота коротка.
   Будить її tick hook (debounce у SysTick), більше ніхто. */
static void task_btn(void *arg)
{
  app_t *a = arg;
  for (;;) {
    os_wait(OS_FOREVER);
    while (app_btn_pending()) app_btn_run(a);
  }
}

/* imu (5): жорсткий період 5 мс (FR-01); vTaskDelayUntil — без накопичення похибки.
   Джитер міряється одразу після пробудження (perf_jit_tick в app_imu_run). Тут же раз на
   50 мс — потенціометр (23 мкс) і раз на 1 с — вікно завантаження CPU за часом idle. */
static void task_imu(void *arg)
{
  app_t *a = arg;
  uint32_t last = os_ticks(), pot_t = plat_millis();
  for (;;) {
    uint32_t per = (a->imu_period_us * os_tick_hz() + 999999u) / 1000000u;
    os_delay_until(&last, per ? per : 1u);
    app_imu_run(a);
    /* П-28: при збої шини кожне читання — це таймаут драйвера (≈ 34 мс активного очікування).
       З пріоритетом 5 і періодом 5 мс imu тоді забирав би весь CPU (телеметрія, CLI мовчать —
       сценарій 6.2.4). Повтор не частіше ніж раз на IMU_RETRY_MS. */
    if (a->imu_ok && a->imu_status != ST_OK) { os_delay_ms(IMU_RETRY_MS); last = os_ticks(); }
    uint32_t ms = plat_millis();
    if ((int32_t)(ms - pot_t) >= 0) { pot_t = ms + POT_PERIOD_MS; app_pot_run(a); }
    uint32_t now = plat_us(), win = now - a->load_win_t0;
    if (win >= 1000000u) {
      uint32_t idle = os_idle_us() - a->load_idle0;
      uint32_t busy = idle >= win ? 0 : win - idle;
      perf_load_set(&a->load, (uint8_t)((busy / (win / 100u)) > 100u ? 100u : busy / (win / 100u)));
      a->load_win_t0 = now;
      a->load_idle0 = os_idle_us();
    }
  }
}

/* journal (4): REC — споживач черги q_rec (SD повільна: 4.5 мс на блок, але черга на
   8 записів = 160 мс запасу); PLAY — власний період 20 мс (FR-08). */
static void task_journal(void *arg)
{
  app_t *a = arg;
  uint32_t last = os_ticks();
  for (;;) {
    if (a->mode == MODE_PLAY && !a->play_done) {
      os_delay_until(&last, ms_ticks(REC_PERIOD_MS));
      app_play_run(a);
    } else {
      if (a->mode == MODE_REC) app_rec_drain(a, REC_PERIOD_MS);
      else os_wait(REC_PERIOD_MS);              /* app_set_mode(PLAY) будить сповіщенням */
      last = os_ticks();
    }
  }
}

/* cmd (3): прокидається від RX ISR на кінець рядка; таймаут 100 мс — страховка на випадок
   рядка, що прийшов, поки задача обробляла попередній */
static void task_cmd(void *arg)
{
  app_t *a = arg;
#ifdef LR4_DIAG
  { void os_diag_crit(void); os_diag_crit(); }
#endif
  for (;;) {
    os_wait(100);
    app_cmd_run(a);
  }
}

static void task_telem(void *arg)
{
  app_t *a = arg;
  uint32_t last = os_ticks();
  for (;;) {
    uint32_t ms = a->telem_ms;                 /* одне читання: cmd може змінити його між двома (П-26) */
    os_delay_until(&last, ms_ticks(ms ? ms : 100u));
    if (a->telem_ms) app_telem_run(a);
  }
}

/* oled (2): режим 0 — увесь кадр за виклик (≈ 63 мс), як у ЛР3; витісняється imu/journal/
   cmd/btn між транзакціями I²C (м'ютекс береться на кожну транзакцію) */
static void task_oled(void *arg)
{
  app_t *a = arg;
  uint32_t last = os_ticks();
  for (;;) {
    uint32_t steps = a->oled_mode == 0 ? 1u : (a->oled_mode == 1 ? 4u : 20u);
    os_delay_until(&last, ms_ticks(OLED_PERIOD_MS / steps));
    app_oled_run(a);
  }
}

/* long_op (1): довгі операції з черги q_long; `load N` — штучна робота раз на тік */
static void task_long(void *arg)
{
  app_t *a = arg;
  for (;;) {
    int op;
    if (os_queue_recv(a->q_long, &op, a->busy_us ? 1u : OS_FOREVER)) app_long_run(a, op);
    if (a->busy_us) {
      uint32_t t0 = plat_cycles(), need = a->busy_us * plat_cyc_per_us();
      while (plat_cycles() - t0 < need) { }
    }
  }
}

static os_task_t make(void (*fn)(void *), const char *name, uint16_t words, app_t *a, unsigned prio)
{
  os_task_t t = os_task_new(fn, name, words, a, prio);
  if (!t) plat_log("FATAL os_task_new %s\r\n", name);
  return t;
}

#ifdef LR4_DIAG
void os_diag_primask(void);
#endif
void app_rtos_start(app_t *a)
{
#ifdef LR4_DIAG
  os_diag_primask();
#endif
  app_os_objects(a);
  a->t_btn     = make(task_btn,     "btn",     STK_BTN,     a, PRIO_BTN);
  a->t_imu     = make(task_imu,     "imu",     STK_IMU,     a, PRIO_IMU);
  a->t_journal = make(task_journal, "journal", STK_JOURNAL, a, PRIO_JOURNAL);
  a->t_cmd     = make(task_cmd,     "cmd",     STK_CMD,     a, PRIO_CMD);
  a->tk_telem  = make(task_telem,   "telem",   STK_TELEM,   a, PRIO_TELEM);
  a->tk_oled   = make(task_oled,    "oled",    STK_OLED,    a, PRIO_OLED);
  a->t_long    = make(task_long,    "long_op", STK_LONG,    a, PRIO_LONG);
  app_set_btn_task(a->t_btn);
  plat_console_set_waiter(a->t_cmd);
  /* службові піни для хронології (`trace pins`): ті самі канали аналізатора, що в ЛР3 */
  os_trace_pin(a->t_imu, PROBE_IMU);
  os_trace_pin(a->t_journal, PROBE_PWM);
  os_trace_pin(a->t_long, PROBE_LONG);
  os_trace_pin(a->t_btn, PROBE_MARK);
  os_start();
}

/* ================= команди ОС ================= */
static void tasks_report(void)
{
  static os_task_info_t ti[12];          /* не на стеку cmd: 336 Б */
  uint32_t total;
  unsigned n = os_task_report(ti, 12, &total);
  uint32_t n_tick, mn, avg, mx;
  os_tick_cost(&n_tick, &mn, &avg, &mx);
  plat_log("TASKS BEGIN total_us=%lu tick_hz=%lu\r\n", (unsigned long)total, (unsigned long)os_tick_hz());
  for (unsigned i = 0; i < n; i++)
    plat_log("TASK name=%s prio=%u base=%u state=%c run_us=%lu cpu_x10=%lu stack=%lu stack_free_min=%lu\r\n",
             ti[i].name, ti[i].prio, ti[i].base_prio, ti[i].state, (unsigned long)ti[i].run_us,
             (unsigned long)(total >= 1000u ? ti[i].run_us / (total / 1000u) : 0),
             (unsigned long)ti[i].stack_size, (unsigned long)ti[i].stack_free_min);
  plat_log("HEAP total=%lu free=%lu min_ever_free=%lu used_max=%lu malloc_fail=%lu\r\n",
           (unsigned long)os_heap_total(), (unsigned long)os_heap_free(), (unsigned long)os_heap_min_free(),
           (unsigned long)(os_heap_total() - os_heap_min_free()), (unsigned long)os_malloc_failures());
  plat_log("OSTICK n=%lu min_ns=%lu avg_ns=%lu max_ns=%lu\r\n", (unsigned long)n_tick,
           (unsigned long)mn, (unsigned long)avg, (unsigned long)mx);
  plat_log("CTXSW n=%lu per_s=%lu\r\n", (unsigned long)os_ctx_switches(),
           (unsigned long)(total >= 1000000u ? os_ctx_switches() / (total / 1000000u) : 0));
  plat_log("MSP stack_used=%lu stack_size=%lu\r\n", (unsigned long)plat_stack_used(),
           (unsigned long)plat_stack_size());
  plat_log("TASKS END\r\n");
}

static void trace_dump(void)
{
  unsigned n = os_trace_count();
  plat_log("TRACE BEGIN n=%u\r\n", n);
  for (unsigned i = 0; i < n; i++) {
    uint32_t t; const char *name;
    if (os_trace_get(i, &t, &name)) plat_log("TR t_us=%lu task=%s\r\n", (unsigned long)t, name);
  }
  plat_log("TRACE END\r\n");
}

#ifdef LR4_OSTEST
/* Тестовий стенд сценаріїв §7.2 — окрема збірка (make OSTEST=1): разом з ядром не вміщується
   в 32 КБ Flash продуктової прошивки, а продукту ці команди не потрібні. */
/* ---------- 7.2.1 інверсія пріоритетів ----------
   L (пріоритет 1) захоплює ресурс і 50 мс «працює» з ним; H (5) хоче той самий ресурс;
   M (4) у цей час 300 мс займає процесор без блокування. З м'ютексом L успадковує
   пріоритет H і витісняє M — H чекає ≈ 50 мс; з двійковим семафором L стоїть, доки M не
   закінчить, — H чекає ≈ 350 мс. */
static struct {
  os_mutex_t res;
  os_task_t  cmd;
  volatile uint32_t h_wait_us, l_hold_us;
  volatile int done;
} inv;

static void spin_us(uint32_t us)
{
  uint32_t t0 = plat_cycles(), need = us * plat_cyc_per_us();
  while (plat_cycles() - t0 < need) { }
}

static void inv_low(void *arg)
{
  (void)arg;
  os_lock(inv.res, OS_FOREVER);
  os_notify(inv.cmd);                     /* «ресурс зайнято» */
  uint32_t t0 = plat_us();
  spin_us(50000u);                        /* 50 мс CPU-роботи (у кожному тіку може бути витіснена) */
  inv.l_hold_us = plat_us() - t0;
  os_unlock(inv.res);
  for (;;) os_wait(OS_FOREVER);
}

static void inv_med(void *arg)
{
  (void)arg;
  os_wait(OS_FOREVER);                    /* старт разом з H (os_suspend_all) */
  spin_us(300000u);
  inv.done |= 2;
  for (;;) os_wait(OS_FOREVER);
}

static void inv_high(void *arg)
{
  (void)arg;
  os_wait(OS_FOREVER);
  uint32_t t0 = plat_us();
  os_lock(inv.res, OS_FOREVER);
  inv.h_wait_us = plat_us() - t0;
  os_unlock(inv.res);
  inv.done |= 1;
  os_notify(inv.cmd);
  for (;;) os_wait(OS_FOREVER);
}

static app_t *g_app;
static void os_inversion(bool mutex)
{
  /* Продукт завантажує CPU на ~90 %: L (пріоритет 1) не отримав би часу захопити ресурс.
     Періодичні задачі продукту на час досліду призупинено — інверсію видно «в чистому вигляді». */
  os_task_t quiet[] = { g_app->t_imu, g_app->tk_oled, g_app->tk_telem, g_app->t_journal };
  for (unsigned i = 0; i < 4; i++) os_task_suspend(quiet[i]);
  memset((void *)&inv, 0, sizeof inv);
  inv.cmd = os_task_self();
  inv.res = os_mutex_new(mutex);
  os_task_t h = os_task_new(inv_high, "invH", 56, NULL, 5);   /* обидві одразу чекають старту */
  os_task_t m = os_task_new(inv_med, "invM", 56, NULL, 4);
  os_task_t l = os_task_new(inv_low, "invL", 56, NULL, 1);
  os_wait(1000);                          /* L захопив ресурс і почав працювати */
  /* H і M стають готовими одночасно: першим виконується H (5) і блокується на ресурсі;
     далі між L і M вирішує успадкування пріоритету */
  os_suspend_all();
  os_notify(m);
  os_notify(h);
  os_resume_all();
  while ((inv.done & 3) != 3) os_delay_ms(10);
  plat_log("INV kind=%s h_wait_us=%lu l_hold_us=%lu m_busy_us=300000 heap_ok=%d\r\n",
           mutex ? "mutex" : "binsem", (unsigned long)inv.h_wait_us, (unsigned long)inv.l_hold_us,
           l && h && m);
  if (l) os_task_delete(l);
  if (h) os_task_delete(h);
  if (m) os_task_delete(m);
  os_mutex_delete(inv.res);
  os_delay_ms(5);                         /* idle звільняє пам'ять видалених задач */
  for (unsigned i = 0; i < 4; i++) os_task_resume(quiet[i]);
}

/* ---------- 7.2.3 голодування: задача з найвищим пріоритетом не блокується ---------- */
static volatile uint32_t starve_ms;
static void starve_task(void *arg)
{
  (void)arg;
  spin_us(starve_ms * 1000u);
  os_task_delete(NULL);
}

/* ---------- 7.2.4 вичерпання черги: виробник швидший за споживача ---------- */
static struct { os_queue_t q; volatile uint32_t got; } qf;
static void qf_consumer(void *arg)
{
  (void)arg;
  uint32_t v;
  for (;;) if (os_queue_recv(qf.q, &v, OS_FOREVER)) { qf.got++; os_delay_ms(10); }   /* 10 мс на елемент */
}

static void os_qflood(uint32_t n)
{
  qf.q = os_queue_new(8, sizeof(uint32_t));
  qf.got = 0;
  os_task_t c = os_task_new(qf_consumer, "qcons", 80, NULL, 2);
  uint32_t sent = 0, dropped = 0, t0 = plat_us();
  for (uint32_t i = 0; i < n; i++) {      /* 1 подія/мс, без очікування: повна черга = втрата */
    if (os_queue_send(qf.q, &i, 0)) sent++; else dropped++;
    os_delay_ms(1);
  }
  uint32_t t_nb = plat_us() - t0, max_block = 0, sent2 = 0;
  for (uint32_t i = 0; i < n; i++) {      /* те саме з блокуванням до 50 мс */
    uint32_t t1 = plat_us();
    if (os_queue_send(qf.q, &i, 50)) sent2++;
    uint32_t d = plat_us() - t1;
    if (d > max_block) max_block = d;
    os_delay_ms(1);
  }
  plat_log("QFLOOD n=%lu len=8 consumer_ms=10 nb_sent=%lu nb_dropped=%lu nb_us=%lu "
           "blk_sent=%lu blk_max_wait_us=%lu consumed=%lu\r\n", (unsigned long)n, (unsigned long)sent,
           (unsigned long)dropped, (unsigned long)t_nb, (unsigned long)sent2, (unsigned long)max_block,
           (unsigned long)qf.got);
  os_delay_ms(100);
  if (c) os_task_delete(c);
  os_queue_delete(qf.q);
  os_delay_ms(5);
}

/* ---------- 7.2.5 вичерпання heap під час роботи ---------- */
static void os_heap_exhaust(void)
{
  static void *blk[48];
  unsigned n = 0;
  uint32_t f0 = os_malloc_failures(), free0 = os_heap_free();
  while (n < 48 && (blk[n] = os_malloc(128)) != NULL) n++;
  uint32_t free_min = os_heap_free();
  /* те, що відбувається з продуктом, коли heap вичерпано: нова задача/черга не створюється */
  os_queue_t q = os_queue_new(4, 4);
  plat_log("HEAPX blocks=%u block=128 free_before=%lu free_after=%lu hook_calls=%lu queue_create=%s\r\n",
           n, (unsigned long)free0, (unsigned long)free_min, (unsigned long)(os_malloc_failures() - f0),
           q ? "OK" : "FAIL");
  if (q) os_queue_delete(q);
  while (n) os_free(blk[--n]);
  plat_log("HEAP free=%lu min_ever_free=%lu\r\n", (unsigned long)os_heap_free(),
           (unsigned long)os_heap_min_free());
}

/* ---------- 7.2.2 переповнення стека ---------- */
static void __attribute__((noinline)) eat_stack(volatile uint8_t *p, unsigned depth)
{
  volatile uint8_t buf[64];
  for (unsigned i = 0; i < sizeof buf; i++) buf[i] = (uint8_t)i;
  if (depth) eat_stack(buf, depth - 1);
  if (p) p[0] = buf[1];
}

static void ovf_task(void *arg)
{
  (void)arg;
  eat_stack(NULL, 4);                     /* ≈ 5 × 80 Б = 400 Б при стеку 256 Б */
  os_delay_ms(1);                         /* перемикання → перевірка configCHECK_FOR_STACK_OVERFLOW */
  plat_log("OVF survived (check disabled?)\r\n");
  for (;;) os_wait(OS_FOREVER);
}

static void os_overflow(void)
{
  /* «прокладка» нижче стека задачі: heap_4 виділяє стек перед TCB, і переповнення пише вниз —
     у цей блок, а не в TCB/стек іншої задачі; сам хук ядра звідси спрацює раніше, ніж щось зламається */
  void *pad = os_malloc(512);
  plat_log("OVF start stack_words=64 need_bytes=~400 pad=%s\r\n", pad ? "OK" : "FAIL");
  os_task_new(ovf_task, "ovf", 64, NULL, 2);
  os_delay_ms(200);
  plat_log("OVF end (no hook)\r\n");
}

/* ---------- вартість перемикання контексту: notify з нижчого → вищий ---------- */
static struct { volatile uint32_t t_give, sum, mn, mx, n; os_task_t hi; } cs;
static void ctx_hi(void *arg)
{
  (void)arg;
  for (;;) {
    os_wait(OS_FOREVER);
    uint32_t d = plat_cycles() - cs.t_give;
    cs.n++; cs.sum += d;
    if (d < cs.mn) cs.mn = d;
    if (d > cs.mx) cs.mx = d;
  }
}

static void os_ctxsw(void)
{
  cs.n = cs.sum = cs.mx = 0;
  cs.mn = 0xFFFFFFFFu;
  cs.hi = os_task_new(ctx_hi, "ctxhi", 80, NULL, 6);
  for (int i = 0; i < 200; i++) {
    cs.t_give = plat_cycles();
    os_notify(cs.hi);                    /* вищий пріоритет → витіснення одразу в xTaskNotifyGive */
    os_delay_ms(1);
  }
  uint32_t k = plat_cyc_per_us();
  plat_log("CTXSW_COST n=%lu min_cyc=%lu avg_cyc=%lu max_cyc=%lu avg_ns=%lu\r\n", (unsigned long)cs.n,
           (unsigned long)cs.mn, (unsigned long)(cs.n ? cs.sum / cs.n : 0), (unsigned long)cs.mx,
           (unsigned long)(cs.n ? (cs.sum / cs.n) * 1000u / k : 0));
  os_task_delete(cs.hi);
  os_delay_ms(5);
}

static uint32_t num(const char *p) { uint32_t v = 0; while (*p >= '0' && *p <= '9') v = v * 10u + (uint32_t)(*p++ - '0'); return v; }
#endif /* LR4_OSTEST */

bool app_os_cmd(app_t *a, const char *cmd, char *arg)
{
#ifdef LR4_OSTEST
  g_app = a;
#endif
  (void)a;
  if (!strcmp(cmd, "tasks")) { tasks_report(); return true; }
  if (!strcmp(cmd, "trace")) {
    if (!strcmp(arg, "start")) os_trace_start(false);
    else if (!strcmp(arg, "pins")) os_trace_start(true);
    else if (!strcmp(arg, "stop")) os_trace_stop();
    else trace_dump();
    plat_log("OK trace\r\n");
    return true;
  }
  if (strcmp(cmd, "os")) return false;
#ifndef LR4_OSTEST
  plat_log("ERR os tests: build with OSTEST=1\r\n");
  return true;
#else
  char *sub = arg, *p = strchr(arg, ' ');
  if (p) *p++ = '\0'; else p = arg + strlen(arg);
  if (!strcmp(sub, "inv")) os_inversion(strcmp(p, "sem") != 0);
  else if (!strcmp(sub, "starve")) {
    starve_ms = num(p);
    if (!starve_ms) starve_ms = 500;
    plat_log("STARVE start ms=%lu\r\n", (unsigned long)starve_ms);
    if (!os_task_new(starve_task, "starve", 64, NULL, 6)) plat_log("ERR heap\r\n");
    os_delay_ms(starve_ms + 20);
    plat_log("STARVE end\r\n");
  }
  else if (!strcmp(sub, "qflood")) { uint32_t n = num(p); os_qflood(n ? n : 50u); }
  else if (!strcmp(sub, "heap")) os_heap_exhaust();
  else if (!strcmp(sub, "overflow")) os_overflow();
  else if (!strcmp(sub, "ctxsw")) os_ctxsw();
  else { plat_log("ERR os '%s'\r\n", sub); return true; }
  plat_log("OK os\r\n");
  return true;
#endif
}
