/**
 * plat_stm32.c — реалізація Interfaces/plat.h для STM32C0 (HAL).
 */
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "plat.h"

/* ---------- час ---------- */
uint32_t plat_millis(void)         { return HAL_GetTick(); }
void     plat_delay_ms(uint32_t ms) { HAL_Delay(ms); }

/* ---------- консоль ---------- */
void plat_log(const char *fmt, ...)
{
  static char buf[192];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  if (n >= (int)sizeof buf) n = sizeof buf - 1;
  HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)n, 100);
}

#define RX_RING 64u
static volatile uint8_t  rx_ring[RX_RING];
static volatile uint16_t rx_head, rx_tail;

/* Прийом консолі — напряму з регістрів USART2, без HAL_UART_IRQHandler:
   той тягне ~3 КБ (RxISR_*_FIFOEN, гілки DMA-abort), а Flash у C031 лише 32 КБ
   (журнал проблем, П-3). Викликається з USART2_IRQHandler (Core/Src/stm32c0xx_it.c). */
void board_uart_rx_irq(void)
{
  uint32_t isr = USART2->ISR;
  if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
    USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
  if (isr & USART_ISR_RXNE_RXFNE) {
    uint8_t c = (uint8_t)USART2->RDR;           /* читання RDR скидає RXNE */
    uint16_t nx = (uint16_t)((rx_head + 1u) % RX_RING);
    if (nx != rx_tail) { rx_ring[rx_head] = c; rx_head = nx; }
  }
}

bool plat_console_getline(char *buf, size_t size)
{
  static size_t len;
  while (rx_tail != rx_head) {
    char c = (char)rx_ring[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) % RX_RING);
    if (c == '\r' || c == '\n') {
      if (len == 0) continue;
      buf[len] = '\0';
      len = 0;
      return true;
    }
    if (len + 1 < size) buf[len++] = c;
  }
  return false;
}

/* ---------- GPIO ---------- */
void plat_mark(bool level)
{
  HAL_GPIO_WritePin(BOARD_MARK_Port, BOARD_MARK_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void plat_led_toggle(void) { HAL_GPIO_TogglePin(BOARD_LED_Port, BOARD_LED_Pin); }

static volatile uint32_t btn_presses;
int      plat_btn_level(void)   { return HAL_GPIO_ReadPin(BOARD_BTN_Port, BOARD_BTN_Pin) == GPIO_PIN_SET; }
uint32_t plat_btn_presses(void) { return btn_presses; }

/* Кнопка: EXTI по спаду, debounce 50 мс (FR-06) */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin)
{
  static uint32_t last;
  if (pin == BOARD_BTN_Pin) {
    uint32_t now = HAL_GetTick();
    if (now - last > 50u) { btn_presses++; last = now; }
  }
}

/* ---------- PWM ---------- */
void plat_servo_set_us(unsigned ch, uint16_t us)
{
  __HAL_TIM_SET_COMPARE(&htim3, ch ? TIM_CHANNEL_2 : TIM_CHANNEL_1, us);
}

uint16_t plat_servo_get_us(unsigned ch)
{
  return (uint16_t)__HAL_TIM_GET_COMPARE(&htim3, ch ? TIM_CHANNEL_2 : TIM_CHANNEL_1);
}

/* ---------- АЦП ---------- */
int plat_pot_read_raw(void)
{
  int v = -1;
  if (HAL_ADC_Start(&hadc1) == HAL_OK) {
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) v = (int)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
  }
  return v;
}

/* ---------- ідентифікація ---------- */
const char *plat_board_name(void) { return BOARD_NAME; }
uint32_t    plat_sysclk_hz(void)  { return HAL_RCC_GetSysClockFreq(); }

const char *board_hal_str(HAL_StatusTypeDef s)
{
  switch (s) {
    case HAL_OK:      return "HAL_OK";
    case HAL_ERROR:   return "HAL_ERROR";
    case HAL_BUSY:    return "HAL_BUSY";
    case HAL_TIMEOUT: return "HAL_TIMEOUT";
    default:          return "?";
  }
}

void board_start(void)
{
  plat_log("[init] %-14s %s\r\n", "ADC1 calib", board_hal_str(HAL_ADCEx_Calibration_Start(&hadc1)));
  plat_log("[init] %-14s %s\r\n", "PWM CH1", board_hal_str(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1)));
  plat_log("[init] %-14s %s\r\n", "PWM CH2", board_hal_str(HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2)));
  SET_BIT(USART2->CR1, USART_CR1_RXNEIE_RXFNEIE);   /* переривання на прийом байта */
  plat_log("[init] %-14s %s\r\n", "UART RX IRQ", "on");
  plat_log("[init] %-14s %d (0=HAL preload, 1=TXIS-paced, 2=Mem_Write)\r\n", "I2C write mode",
           plat_i2c_get_write_mode());
  plat_mark(false);
}
