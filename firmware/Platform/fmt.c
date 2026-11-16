/**
 * fmt.c — мінімальний форматер (підмножина printf) для plat_log.
 * Чиста функція без HAL: збирається і для МК, і в хостових тестах (tests/test_fmt.c
 * порівнює з snprintf). Замінює vsnprintf newlib-nano (~2.5 КБ Flash разом з malloc,
 * журнал ЛР3 П-13). Підтримка: %[-0][width][l]{d,u,x,X,s,c,%}.
 */
#include "plat.h"

typedef struct { char *p; size_t left, n; } sink_t;

static void put(sink_t *s, char c)
{
  if (s->left > 1) { *s->p++ = c; s->left--; }
  s->n++;
}

static void put_num(sink_t *s, uint32_t v, unsigned base, int neg, int width, char pad,
                    int left, int upper)
{
  char tmp[11];
  int i = 0;
  const char *dig = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  do { tmp[i++] = dig[v % base]; v /= base; } while (v);
  int len = i + (neg ? 1 : 0);
  if (neg && pad == '0') { put(s, '-'); neg = 0; }
  if (!left) for (; len < width; len++) put(s, pad);
  if (neg) put(s, '-');
  while (i) put(s, tmp[--i]);
  if (left) for (; len < width; len++) put(s, ' ');
}

size_t plat_vfmt(char *buf, size_t size, const char *f, va_list ap)
{
  sink_t s = { buf, size, 0 };
  for (; *f; f++) {
    if (*f != '%') { put(&s, *f); continue; }
    f++;
    int left = 0, width = 0;
    char pad = ' ';
    if (*f == '-') { left = 1; f++; }
    if (*f == '0') { pad = '0'; f++; }
    while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
    if (*f == 'l') f++;                   /* long == int на ARMv6-M */
    switch (*f) {
      case 'd': {
        int32_t v = va_arg(ap, int32_t);
        put_num(&s, v < 0 ? (uint32_t)(-v) : (uint32_t)v, 10, v < 0, width, pad, left, 0);
        break;
      }
      case 'u': put_num(&s, va_arg(ap, uint32_t), 10, 0, width, pad, left, 0); break;
      case 'x': put_num(&s, va_arg(ap, uint32_t), 16, 0, width, pad, left, 0); break;
      case 'X': put_num(&s, va_arg(ap, uint32_t), 16, 0, width, pad, left, 1); break;
      case 'c': put(&s, (char)va_arg(ap, int)); break;
      case 's': {
        const char *str = va_arg(ap, const char *);
        int len = 0;
        if (!str) str = "(null)";
        while (str[len]) len++;
        if (!left) for (; len < width; width--) put(&s, ' ');
        while (*str) put(&s, *str++);
        if (left) for (; len < width; width--) put(&s, ' ');
        break;
      }
      case '%': put(&s, '%'); break;
      case '\0': f--; break;
      default: put(&s, '%'); put(&s, *f); break;
    }
  }
  if (size) *s.p = '\0';
  return s.n;
}

size_t plat_fmt(char *buf, size_t size, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  size_t n = plat_vfmt(buf, size, fmt, ap);
  va_end(ap);
  return n;
}

