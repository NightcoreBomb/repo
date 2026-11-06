#include "button.h"

void button_init(button_t *b, int level, uint32_t now_ms, uint16_t debounce_ms)
{
  b->stable = b->raw = level ? 1u : 0u;
  b->t_raw = now_ms;
  b->debounce_ms = debounce_ms;
  b->presses = b->releases = 0;
}

btn_event_t button_update(button_t *b, int level, uint32_t now_ms)
{
  uint8_t l = level ? 1u : 0u;
  if (l != b->raw) {                 /* будь-який фронт (у т. ч. брязкіт) перезапускає вікно */
    b->raw = l;
    b->t_raw = now_ms;
    return BTN_EV_NONE;
  }
  if (l == b->stable || (uint32_t)(now_ms - b->t_raw) < b->debounce_ms)
    return BTN_EV_NONE;
  b->stable = l;                     /* рівень стабільний ≥ debounce_ms — приймаємо */
  if (l == 0) { b->presses++;  return BTN_EV_PRESS; }
  b->releases++;
  return BTN_EV_RELEASE;
}
