/**
 * os.h — сервіси ОС для прикладного шару (ЛР4).
 *
 * App не включає заголовки FreeRTOS (правило шарів, tools/check_deps.py): усе, що йому
 * потрібно від ядра, — тут. Реалізація на STM32 — Platform/os_freertos.c (FreeRTOS V11.1.0);
 * у хостових тестах — tests/doubles/fake_os.c (однопотокова: м'ютекси завжди вільні,
 * черги — кільця, задачі не створюються).
 */
#ifndef IF_OS_H
#define IF_OS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *os_task_t;
typedef void *os_mutex_t;
typedef void *os_queue_t;

#define OS_FOREVER 0xFFFFFFFFu

/* ---------- задачі ---------- */
os_task_t os_task_new(void (*fn)(void *), const char *name, uint16_t stack_words, void *arg,
                      unsigned prio);
void      os_task_delete(os_task_t t);
void      os_task_suspend(os_task_t t);           /* сценарії ОС: ізолювати експеримент від продукту */
void      os_task_resume(os_task_t t);              /* NULL — поточна */
os_task_t os_task_self(void);
void      os_start(void) __attribute__((__noreturn__));
bool      os_running(void);

/* ---------- час ---------- */
uint32_t  os_tick_hz(void);
uint32_t  os_ticks(void);
uint32_t  os_ms_to_ticks(uint32_t ms);              /* ≥ 1 тік для ms > 0 */
void      os_delay_ms(uint32_t ms);
/* Періодичне пробудження без накопичення похибки (vTaskDelayUntil). *last — тіки. */
void      os_delay_until(uint32_t *last, uint32_t period_ticks);
void      os_yield(void);
void      os_suspend_all(void);                     /* зупинити планувальник (переривання працюють) */
void      os_resume_all(void);

/* ---------- сповіщення (одна подія → одна задача) ---------- */
void      os_notify(os_task_t t);
void      os_notify_isr(os_task_t t);
bool      os_wait(uint32_t ms);                     /* true — прийшло сповіщення */

/* ---------- м'ютекси / двійкові семафори ---------- */
/* inherit = true — м'ютекс з успадкуванням пріоритету; false — двійковий семафор
   (для сценарію 7.2.1 «інверсія пріоритетів») */
os_mutex_t os_mutex_new(bool inherit);
void       os_mutex_delete(os_mutex_t m);
bool       os_lock(os_mutex_t m, uint32_t ms);
void       os_unlock(os_mutex_t m);

/* ---------- черги ---------- */
os_queue_t os_queue_new(unsigned len, unsigned item_size);
void       os_queue_delete(os_queue_t q);
bool       os_queue_send(os_queue_t q, const void *item, uint32_t ms);
bool       os_queue_recv(os_queue_t q, void *item, uint32_t ms);
unsigned   os_queue_count(os_queue_t q);

/* ---------- пам'ять ядра ---------- */
void      *os_malloc(size_t n);
void       os_free(void *p);
uint32_t   os_heap_total(void);
uint32_t   os_heap_free(void);
uint32_t   os_heap_min_free(void);
uint32_t   os_malloc_failures(void);                /* скільки разів спрацював MallocFailedHook */

/* ---------- діагностика ---------- */
typedef struct {
  const char *name;
  unsigned    prio, base_prio;
  char        state;                                /* R(unning) r(eady) B(locked) S(uspended) D(eleted) */
  uint32_t    run_us;                               /* run-time stats від `stats reset` */
  uint32_t    stack_free_min;                       /* high water mark, байтів */
  uint32_t    stack_size;                           /* байтів (як створено) */
} os_task_info_t;

unsigned  os_task_report(os_task_info_t *out, unsigned max, uint32_t *total_us);
void      os_stats_reset(void);                     /* нуль для run-time stats і лічильників ядра */
uint32_t  os_ctx_switches(void);                    /* перемикань з `stats reset` */
uint32_t  os_idle_us(void);                         /* час у idle з `stats reset` */
/* Вартість тіку: тривалість обробника SysTick (НАНОсекунди — він коротший за 1 мкс), з `stats reset` */
void      os_tick_cost(uint32_t *n, uint32_t *min_ns, uint32_t *avg_ns, uint32_t *max_ns);

/* Хронологія: кільце з N останніх перемикань після `trace start` (одноразове заповнення) */
void      os_trace_start(bool pins);
void      os_trace_stop(void);
unsigned  os_trace_count(void);
bool      os_trace_get(unsigned i, uint32_t *t_us, const char **name);
/* Призначити задачі службовий пін для режиму `trace pins` (0..3 = PROBE_*, -1 — без піна) */
void      os_trace_pin(os_task_t t, int probe);

#endif
