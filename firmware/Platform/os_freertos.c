/**
 * os_freertos.c — реалізація Interfaces/os.h на FreeRTOS-Kernel V11.1.0 (порт ARM_CM0).
 *
 * Тут же — усе, що потрібно ЛР4 для вимірювань ОС (методичка §6):
 *   run-time stats за задачами (лічильник — plat_us), high water mark стеків, heap_4,
 *   вартість тіку (тривалість SysTick_Handler у тактах), кількість перемикань контексту,
 *   хронологія перемикань (traceTASK_SWITCHED_IN → кільце + службові піни),
 *   хуки configASSERT / переповнення стека / вичерпання heap.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"
#include "os.h"
#include "plat.h"

/* ---------------- задачі ---------------- */
#define OS_MAX_TASKS 12u
static struct { TaskHandle_t h; int8_t probe; uint16_t stack_words; } reg[OS_MAX_TASKS];

static void run_t0_clear(unsigned i);
static void reg_add(TaskHandle_t h, uint16_t words)
{
  for (unsigned i = 0; i < OS_MAX_TASKS; i++)
    if (!reg[i].h) { reg[i].h = h; reg[i].probe = -1; reg[i].stack_words = words; run_t0_clear(i); return; }
}

os_task_t os_task_new(void (*fn)(void *), const char *name, uint16_t stack_words, void *arg,
                      unsigned prio)
{
  TaskHandle_t h = NULL;
  if (xTaskCreate(fn, name, stack_words, arg, prio, &h) != pdPASS) return NULL;
  taskENTER_CRITICAL();
  reg_add(h, stack_words);
  taskEXIT_CRITICAL();
  return h;
}

void os_task_delete(os_task_t t)
{
  TaskHandle_t h = t ? (TaskHandle_t)t : xTaskGetCurrentTaskHandle();
  taskENTER_CRITICAL();
  for (unsigned i = 0; i < OS_MAX_TASKS; i++) if (reg[i].h == h) reg[i].h = NULL;
  taskEXIT_CRITICAL();
  vTaskDelete(h);
}

os_task_t os_task_self(void) { return xTaskGetCurrentTaskHandle(); }
bool      os_running(void)   { return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED; }

/* ---------------- консоль: м'ютекс UART (plat_log) ---------------- */
static SemaphoreHandle_t uart_mx;

void os_uart_lock(void)
{
  if (uart_mx && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) xSemaphoreTake(uart_mx, portMAX_DELAY);
}

void os_uart_unlock(void)
{
  if (uart_mx && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) xSemaphoreGive(uart_mx);
}

void os_start(void)
{
  uart_mx = xSemaphoreCreateMutex();
  configASSERT(uart_mx);
  vTaskStartScheduler();                       /* повертається лише якщо не вистачило heap на idle */
  plat_log_raw("FATAL scheduler did not start (heap)\r\n");
  for (;;) { }
}

/* ---------------- час ---------------- */
uint32_t os_tick_hz(void) { return configTICK_RATE_HZ; }
uint32_t os_ticks(void)   { return xTaskGetTickCount(); }

uint32_t os_ms_to_ticks(uint32_t ms)
{
  if (ms == OS_FOREVER) return portMAX_DELAY;
  uint32_t t = (ms * configTICK_RATE_HZ + 999u) / 1000u;   /* вгору: «не раніше ніж через ms» */
  return t;
}

void os_delay_ms(uint32_t ms)                       { vTaskDelay(os_ms_to_ticks(ms)); }
void os_delay_until(uint32_t *last, uint32_t ticks) { vTaskDelayUntil((TickType_t *)last, ticks); }
void os_yield(void)                                 { taskYIELD(); }
void os_suspend_all(void)                           { vTaskSuspendAll(); }
void os_resume_all(void)                            { xTaskResumeAll(); }

/* ---------------- сповіщення ---------------- */
void os_notify(os_task_t t) { if (t) xTaskNotifyGive((TaskHandle_t)t); }
void os_task_suspend(os_task_t t) { if (t) vTaskSuspend((TaskHandle_t)t); }
void os_task_resume(os_task_t t)  { if (t) vTaskResume((TaskHandle_t)t); }

void os_notify_isr(os_task_t t)
{
  /* П-29: до старту — нікого будити; при зупиненому планувальнику ядро саме відкладе пробудження
     (xPendingReadyList), тож сповіщення не можна відкидати — інакше губиться натискання кнопки */
  if (!t || xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) return;
  BaseType_t woken = pdFALSE;
  vTaskNotifyGiveFromISR((TaskHandle_t)t, &woken);
  portYIELD_FROM_ISR(woken);
}

bool os_wait(uint32_t ms) { return ulTaskNotifyTake(pdTRUE, os_ms_to_ticks(ms)) != 0; }

/* ---------------- м'ютекси ---------------- */
os_mutex_t os_mutex_new(bool inherit)
{
  SemaphoreHandle_t s = inherit ? xSemaphoreCreateMutex() : xSemaphoreCreateBinary();
  if (s && !inherit) xSemaphoreGive(s);             /* двійковий семафор створюється «зайнятим» */
  return s;
}

void os_mutex_delete(os_mutex_t m) { if (m) vSemaphoreDelete((SemaphoreHandle_t)m); }

bool os_lock(os_mutex_t m, uint32_t ms)
{
  if (!m || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return true;   /* до старту — один потік */
  return xSemaphoreTake((SemaphoreHandle_t)m, os_ms_to_ticks(ms)) == pdTRUE;
}

void os_unlock(os_mutex_t m)
{
  if (m && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) xSemaphoreGive((SemaphoreHandle_t)m);
}

/* ---------------- черги ---------------- */
os_queue_t os_queue_new(unsigned len, unsigned size) { return xQueueCreate(len, size); }
void       os_queue_delete(os_queue_t q)             { if (q) vQueueDelete((QueueHandle_t)q); }
unsigned   os_queue_count(os_queue_t q)              { return q ? (unsigned)uxQueueMessagesWaiting((QueueHandle_t)q) : 0; }

bool os_queue_send(os_queue_t q, const void *item, uint32_t ms)
{
  return xQueueSend((QueueHandle_t)q, item, os_ms_to_ticks(ms)) == pdTRUE;
}

bool os_queue_recv(os_queue_t q, void *item, uint32_t ms)
{
  return xQueueReceive((QueueHandle_t)q, item, os_ms_to_ticks(ms)) == pdTRUE;
}

/* ---------------- пам'ять ---------------- */
static volatile uint32_t malloc_failures;

void    *os_malloc(size_t n)        { return pvPortMalloc(n); }
void     os_free(void *p)           { vPortFree(p); }
uint32_t os_heap_total(void)        { return configTOTAL_HEAP_SIZE; }
uint32_t os_heap_free(void)         { return xPortGetFreeHeapSize(); }
uint32_t os_heap_min_free(void)     { return xPortGetMinimumEverFreeHeapSize(); }
uint32_t os_malloc_failures(void)   { return malloc_failures; }

/* Сценарій 7.2.5: хук викликається з pvPortMalloc при зупиненому планувальнику —
   друкуємо без м'ютекса і повертаємось: викликач отримає NULL і має це обробити. */
void vApplicationMallocFailedHook(void)
{
  malloc_failures++;
  plat_log_raw("HOOK malloc_failed free=%u\r\n", (unsigned)xPortGetFreeHeapSize());
}

/* Сценарій 7.2.2: переповнення стека задачі (configCHECK_FOR_STACK_OVERFLOW = 2:
   перевірка вказівника стека і 16 байтів шаблону на межі при кожному перемиканні) */
void vApplicationStackOverflowHook(TaskHandle_t t, char *name)
{
  (void)t;
  taskDISABLE_INTERRUPTS();
  plat_log_raw("FATAL stack overflow task=%s\r\n", name);
  for (;;) { }
}

void os_assert_failed(const char *file, int line)
{
  taskDISABLE_INTERRUPTS();
  plat_log_raw("FATAL assert %s:%d task=%s\r\n", file, line,
               xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED ? "-" : pcTaskGetName(NULL));
  for (;;) { }
}

/* ---------------- вартість тіку і перемикань ---------------- */
static volatile uint32_t ctx_sw, tick_n, tick_sum_cyc, tick_min_cyc = 0xFFFFFFFFu, tick_max_cyc;
static volatile uint32_t stats_t0_us, idle_t0;

/* Викликається з SysTick_Handler: v0 — SysTick->VAL на вході. Рахуємо по самому VAL
   (лічить донизу), бо uwTick ще не збільшено і plat_cycles() дав би похибку в 1 тік. */
void os_tick_account(uint32_t v0)
{
  uint32_t d = v0 - SysTick->VAL;
  if (d > SysTick->LOAD) return;                    /* пропустили перезавантаження — не рахуємо */
  tick_n++;
  tick_sum_cyc += d;
  if (d < tick_min_cyc) tick_min_cyc = d;
  if (d > tick_max_cyc) tick_max_cyc = d;
}

void os_tick_cost(uint32_t *n, uint32_t *mn, uint32_t *avg, uint32_t *mx)
{
  uint32_t k = plat_cyc_per_us();
  *n = tick_n;
  *mn = tick_n ? tick_min_cyc * 1000u / k : 0;      /* у наносекундах — тік коротший за 1 мкс? */
  *avg = tick_n ? (tick_sum_cyc / tick_n) * 1000u / k : 0;
  *mx = tick_max_cyc * 1000u / k;
}

uint32_t os_ctx_switches(void) { return ctx_sw; }

uint32_t os_idle_us(void)
{
  return ulTaskGetIdleRunTimeCounter() - idle_t0;
}

/* ---------------- хронологія ---------------- */
#ifdef LR4_OSTEST
#define TRACE_N 16u                 /* OSTEST: −512 Б RAM на користь heap тестових задач */
#else
#define TRACE_N 48u
#endif
static struct { uint32_t t_us; TaskHandle_t h; } trace_buf[TRACE_N];
static volatile uint8_t trace_n, trace_on, trace_pins;

void os_trace_pin(os_task_t t, int probe)
{
  for (unsigned i = 0; i < OS_MAX_TASKS; i++) if (reg[i].h == t) reg[i].probe = (int8_t)probe;
}

void os_trace_switched_in(void)
{
  ctx_sw++;
  if (!trace_on && !trace_pins) return;
  TaskHandle_t h = xTaskGetCurrentTaskHandle();
  if (trace_pins) {                                 /* пін задачі = 1 поки вона виконується */
    for (unsigned i = 0; i < OS_MAX_TASKS; i++)
      if (reg[i].probe >= 0) plat_probe((unsigned)reg[i].probe, reg[i].h == h);
  }
  if (trace_on && trace_n < TRACE_N) {
    trace_buf[trace_n].t_us = plat_us();
    trace_buf[trace_n].h = h;
    if (++trace_n == TRACE_N) trace_on = 0;
  }
}

void os_trace_start(bool pins) { trace_n = 0; trace_pins = pins; trace_on = 1; }
void os_trace_stop(void)
{
  trace_on = 0;
  if (trace_pins) { trace_pins = 0; for (unsigned p = 0; p < 4; p++) plat_probe(p, false); }
}
unsigned os_trace_count(void) { return trace_n; }

bool os_trace_get(unsigned i, uint32_t *t_us, const char **name)
{
  if (i >= trace_n) return false;
  *t_us = trace_buf[i].t_us;
  *name = "?";
  TaskHandle_t h = trace_buf[i].h;
  if (h == xTaskGetIdleTaskHandle()) *name = "IDLE";
  else for (unsigned k = 0; k < OS_MAX_TASKS; k++) if (reg[k].h == h) *name = pcTaskGetName(h);
  return true;
}

/* ---------------- звіт про задачі ----------------
   Без configUSE_TRACE_FACILITY (uxTaskGetSystemState + масив TaskStatus_t = +0.9 КБ Flash і
   432 Б RAM): обходимо власний реєстр задач + idle, по кожній — API для одного дескриптора. */
static uint32_t run_t0[OS_MAX_TASKS + 1];           /* знімок run-time на `stats reset` */

static TaskHandle_t rep_handle(unsigned i)          /* 0..N-1 — реєстр, N — idle */
{
  return i < OS_MAX_TASKS ? reg[i].h : xTaskGetIdleTaskHandle();
}

void os_stats_reset(void)
{
  for (unsigned i = 0; i <= OS_MAX_TASKS; i++) {
    TaskHandle_t h = rep_handle(i);
    run_t0[i] = h ? ulTaskGetRunTimeCounter(h) : 0;
  }
  idle_t0 = ulTaskGetIdleRunTimeCounter();
  stats_t0_us = plat_us();
  ctx_sw = 0;
  tick_n = tick_sum_cyc = tick_max_cyc = 0;
  tick_min_cyc = 0xFFFFFFFFu;
}

unsigned os_task_report(os_task_info_t *out, unsigned max, uint32_t *total_us)
{
  static const char sc[] = { 'R', 'r', 'B', 'S', 'D', '?' };
  unsigned k = 0;
  for (unsigned i = 0; i <= OS_MAX_TASKS && k < max; i++) {
    TaskHandle_t h = rep_handle(i);
    if (!h) continue;
    eTaskState e = eTaskGetState(h);
    out[k].name = pcTaskGetName(h);
    out[k].prio = (unsigned)uxTaskPriorityGet(h);
    out[k].base_prio = out[k].prio;
    out[k].state = sc[e < 5 ? e : 5];
    out[k].run_us = ulTaskGetRunTimeCounter(h) - run_t0[i];
    out[k].stack_free_min = (uint32_t)uxTaskGetStackHighWaterMark(h) * sizeof(StackType_t);
    out[k].stack_size = (i < OS_MAX_TASKS ? reg[i].stack_words : configMINIMAL_STACK_SIZE) * sizeof(StackType_t);
    k++;
  }
  if (total_us) *total_us = plat_us() - stats_t0_us;
  return k;
}

static void run_t0_clear(unsigned i) { run_t0[i] = 0; }

#ifdef LR4_DIAG
/* П-29: чи маскує PRIMASK (cpsid i) переривання SysTick у симуляторі? До старту планувальника
   SysTick_Handler лише збільшує uwTick, тож зміна uwTick усередині cpsid — пряме свідчення. */
extern volatile uint32_t uwTick;
void os_diag_primask(void)
{
  uint32_t t0 = uwTick;
  __disable_irq();
  for (volatile uint32_t i = 0; i < 200000u; i++) { }
  uint32_t t1 = uwTick;
  __enable_irq();
  for (volatile uint32_t i = 0; i < 1000u; i++) { }
  uint32_t t2 = uwTick;
  for (volatile uint32_t i = 0; i < 200000u; i++) { }
  uint32_t t3 = uwTick;
  plat_log_raw("PRIMASK_TEST ticks_masked=%lu after_enable=%lu unmasked=%lu\r\n",
               (unsigned long)(t1 - t0), (unsigned long)(t2 - t1), (unsigned long)(t3 - t2));
}

/* П-29: те саме вже під планувальником: чи виконуються SysTick і PendSV усередині
   taskENTER_CRITICAL (лічильник тіків і лічильник перемикань не мають змінитися) */
void os_diag_crit(void)
{
  uint32_t k0 = xTaskGetTickCount(), c0 = ctx_sw;
  taskENTER_CRITICAL();
  portYIELD();                                     /* PENDSVSET — має чекати виходу з секції */
  for (volatile uint32_t i = 0; i < 200000u; i++) { }
  uint32_t k1 = xTaskGetTickCount(), c1 = ctx_sw;
  taskEXIT_CRITICAL();
  uint32_t k2 = xTaskGetTickCount(), c2 = ctx_sw;
  plat_log_raw("CRIT_TEST ticks_in=%lu ctxsw_in=%lu ticks_after=%lu ctxsw_after=%lu\r\n",
               (unsigned long)(k1 - k0), (unsigned long)(c1 - c0), (unsigned long)(k2 - k1), (unsigned long)(c2 - c1));
}

/* П-27: idle hook — повна перевірка списків ядра; tick hook — «канарка» стека поточної задачі */
int  os_diag_check(char *out, size_t n);
void os_diag_tick(void);
void vApplicationIdleHook(void)
{
  static char why[80];
  static uint32_t runs;
  if (os_diag_check(why, sizeof why)) {
    taskDISABLE_INTERRUPTS();
    plat_log_raw("FATAL diag %s runs=%lu\r\n", why, (unsigned long)runs);
    for (;;) { }
  }
  runs++;
}
#endif
