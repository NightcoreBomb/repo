/**
 * fake_os.c — Interfaces/os.h для хостових тестів: однопотоковий світ.
 * М'ютекси завжди вільні, черги — кільця в пам'яті, задач і планувальника немає
 * (кроки задач викликає app_poll() у режимі APP_COOP). Лічильники ОС — нулі.
 */
#include <stdlib.h>
#include <string.h>
#include "os.h"

struct fq { unsigned len, size, head, n; unsigned char *buf; };

os_task_t os_task_new(void (*fn)(void *), const char *n, uint16_t w, void *a, unsigned p)
{ (void)fn; (void)n; (void)w; (void)a; (void)p; return NULL; }
void      os_task_delete(os_task_t t) { (void)t; }
os_task_t os_task_self(void) { return NULL; }
void      os_start(void) { abort(); }
bool      os_running(void) { return false; }
uint32_t  os_tick_hz(void) { return 1000; }
uint32_t  os_ticks(void) { return 0; }
uint32_t  os_ms_to_ticks(uint32_t ms) { return ms; }
void      os_delay_ms(uint32_t ms) { (void)ms; }
void      os_delay_until(uint32_t *l, uint32_t t) { *l += t; }
void      os_yield(void) { }
void      os_suspend_all(void) { }
void      os_resume_all(void) { }
void      os_notify(os_task_t t) { (void)t; }
void      os_notify_isr(os_task_t t) { (void)t; }
bool      os_wait(uint32_t ms) { (void)ms; return false; }

static int mx_dummy;
os_mutex_t os_mutex_new(bool inherit) { (void)inherit; return &mx_dummy; }
void       os_mutex_delete(os_mutex_t m) { (void)m; }
bool       os_lock(os_mutex_t m, uint32_t ms) { (void)m; (void)ms; return true; }
void       os_unlock(os_mutex_t m) { (void)m; }

os_queue_t os_queue_new(unsigned len, unsigned size)
{
  struct fq *q = calloc(1, sizeof *q);
  q->len = len; q->size = size; q->buf = calloc(len, size);
  return q;
}
void os_queue_delete(os_queue_t qq) { struct fq *q = qq; if (q) { free(q->buf); free(q); } }
unsigned os_queue_count(os_queue_t qq) { return ((struct fq *)qq)->n; }
bool os_queue_send(os_queue_t qq, const void *item, uint32_t ms)
{
  (void)ms;
  struct fq *q = qq;
  if (q->n == q->len) return false;
  memcpy(q->buf + ((q->head + q->n) % q->len) * q->size, item, q->size);
  q->n++;
  return true;
}
bool os_queue_recv(os_queue_t qq, void *item, uint32_t ms)
{
  (void)ms;
  struct fq *q = qq;
  if (!q->n) return false;
  memcpy(item, q->buf + q->head * q->size, q->size);
  q->head = (q->head + 1) % q->len;
  q->n--;
  return true;
}

void    *os_malloc(size_t n) { return malloc(n); }
void     os_free(void *p) { free(p); }
uint32_t os_heap_total(void) { return 0; }
uint32_t os_heap_free(void) { return 0; }
uint32_t os_heap_min_free(void) { return 0; }
uint32_t os_malloc_failures(void) { return 0; }
unsigned os_task_report(os_task_info_t *o, unsigned m, uint32_t *t) { (void)o; (void)m; if (t) *t = 0; return 0; }
void     os_stats_reset(void) { }
uint32_t os_ctx_switches(void) { return 0; }
uint32_t os_idle_us(void) { return 0; }
void     os_tick_cost(uint32_t *n, uint32_t *a, uint32_t *b, uint32_t *c) { *n = *a = *b = *c = 0; }
void     os_trace_start(bool pins) { (void)pins; }
void     os_trace_stop(void) { }
unsigned os_trace_count(void) { return 0; }
bool     os_trace_get(unsigned i, uint32_t *t, const char **n) { (void)i; (void)t; (void)n; return false; }
void     os_trace_pin(os_task_t t, int p) { (void)t; (void)p; }

/* app_rtos.c на хості не збирається: команди ОС недоступні */
struct app;
bool app_os_cmd(struct app *a, const char *cmd, char *arg) { (void)a; (void)cmd; (void)arg; return false; }
