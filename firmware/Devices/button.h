/**
 * button.h — debounce кнопки за СТАБІЛЬНІСТЮ рівня (чиста логіка, тестується на хості).
 *
 * Стан змінюється лише тоді, коли сирий рівень пробув незмінним ≥ debounce_ms.
 * Натискання = перехід «стабільно 1 (відпущена) → стабільно 0 (натиснута)».
 * Брязкіт і на натисканні, і на ВІДПУСКАННІ не дає зайвих подій (журнал П-12):
 * попередня реалізація («ігнорувати спади 50 мс після попереднього») рахувала
 * спад з брязкоту відпускання як друге натискання, якщо кнопку тримали > 50 мс.
 * Підхід — J. Ganssle, «A Guide to Debouncing», розд. «Software debouncers».
 */
#ifndef DEV_BUTTON_H
#define DEV_BUTTON_H

#include <stdint.h>

#define BUTTON_DEBOUNCE_MS 50u    /* PRD FR-06 */

typedef enum { BTN_EV_NONE = 0, BTN_EV_PRESS = 1, BTN_EV_RELEASE = -1 } btn_event_t;

typedef struct {
  uint8_t  stable;        /* прийнятий рівень: 1 — відпущена (pull-up), 0 — натиснута */
  uint8_t  raw;           /* останній сирий рівень */
  uint32_t t_raw;         /* коли сирий рівень востаннє змінився */
  uint16_t debounce_ms;
  volatile uint32_t presses, releases;
} button_t;

void        button_init(button_t *b, int level, uint32_t now_ms, uint16_t debounce_ms);
/* Викликати періодично (у нас — кожну 1 мс із SysTick) */
btn_event_t button_update(button_t *b, int level, uint32_t now_ms);

#endif
