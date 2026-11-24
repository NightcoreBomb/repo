/**
 * plat_stm32.c — реалізація Interfaces/plat.h для STM32C0 (регістри + HAL_RCC/GPIO).
 *
 * ЛР3: замість vsnprintf (newlib-nano, ~2.5 КБ разом із malloc) — власний форматер
 * plat_vfmt (~0.4 КБ); замість HAL_UART/TIM/ADC — звернення до регістрів.
 * Причина — 32 КБ Flash (журнал ЛР3, П-13).
 */
#include "board.h"
#include "plat.h"
#include "os.h"

extern volatile uint32_t uwTick;          /* HAL: лічильник мс; ЛР4 — інкремент у tick hook FreeRTOS */

/* ================= час ================= */
uint32_t plat_millis(void)         { return HAL_GetTick(); }
/* ЛР4: після старту планувальника очікування — блокування задачі (процесор вільний для інших),
   до старту (ініціалізація пристроїв у app_start) — активне очікування HAL, як у ЛР3 */
void     plat_delay_ms(uint32_t ms) { if (os_running()) os_delay_ms(ms); else HAL_Delay(ms); }
uint32_t plat_cyc_per_us(void)     { return BOARD_SYSCLK_HZ / 1000000u; }

/* Пара (uwTick, VAL) має бути узгоджена: читаємо мс, VAL, ще раз мс. Якщо
   SysTick уже «перекинувся» (COUNTFLAG/PENDSTSET), а обробник ще не виконано
   (ми в ISR вищого пріоритету або в критичній секції), — враховуємо ту мілісекунду. */
static void tick_pair(uint32_t *ms, uint32_t *el)
{
  uint32_t m, v;
  do {
    m = uwTick;
    v = SysTick->VAL;
  } while (m != uwTick);
  uint32_t load = SysTick->LOAD;
  /* uwTickFreq — мс на один тік (1 при 1 кГц, 10 при TICK_HZ=100): формула
     «мс × тактів_на_мс + (LOAD − VAL)» лишається правильною для будь-якої частоти тіку */
  if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) && v > load / 2u) m += uwTickFreq;
  *ms = m;
  *el = load - v;                         /* тактів від початку поточної мс */
}

uint32_t plat_cycles(void)
{
  uint32_t ms, el;
  tick_pair(&ms, &el);
  return ms * (BOARD_SYSCLK_HZ / 1000u) + el;
}

uint32_t plat_us(void)
{
  uint32_t ms, el;
  tick_pair(&ms, &el);
  return ms * 1000u + el / (BOARD_SYSCLK_HZ / 1000000u);
}

/* ================= консоль: TX ================= */
static void uart_putc(char c)
{
  while (!(BOARD_UART->ISR & USART_ISR_TXE_TXFNF)) { }
  BOARD_UART->TDR = (uint8_t)c;
}

/* ЛР4: рядок виводиться під м'ютексом UART (кілька задач пишуть у консоль) — рядки
   різних задач не перемішуються; буфер спільний, тож форматування теж під м'ютексом. */
void plat_log(const char *fmt, ...)
{
  static char buf[160];
  os_uart_lock();
  va_list ap;
  va_start(ap, fmt);
  size_t n = plat_vfmt(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n >= sizeof buf) n = sizeof buf - 1;
  for (size_t i = 0; i < n; i++) uart_putc(buf[i]);
  os_uart_unlock();
}

/* Без м'ютекса — для хуків ядра (переривання вимкнені або планувальник зупинено) */
void plat_log_raw(const char *fmt, ...)
{
  static char buf[96];
  va_list ap;
  va_start(ap, fmt);
  size_t n = plat_vfmt(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n >= sizeof buf) n = sizeof buf - 1;
  for (size_t i = 0; i < n; i++) uart_putc(buf[i]);
}

/* ================= консоль: RX (переривання) ================= */
#define RX_RING 128u
static volatile uint8_t  rx_ring[RX_RING];
static volatile uint16_t rx_head, rx_tail;
static volatile uint32_t rx_dropped, rx_ore, rx_bytes;
/* Момент прийому символу кінця рядка — для латентності «команда → відповідь».
   Кільце з 4 міток: на випадок, коли кілька рядків уже чекають у буфері. */
static volatile uint32_t eol_cyc[4];
static volatile uint8_t  eol_head, eol_tail;
static void *volatile    console_waiter;    /* ЛР4: задача, яку будить кінець рядка */

void plat_console_set_waiter(void *task) { console_waiter = task; }

void board_uart_rx_irq(void)
{
  uint32_t isr = BOARD_UART->ISR;
  if (isr & USART_ISR_ORE) rx_ore++;
  if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
    BOARD_UART->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
  if (isr & USART_ISR_RXNE_RXFNE) {
    uint8_t c = (uint8_t)BOARD_UART->RDR;
    rx_bytes++;
    uint16_t nx = (uint16_t)((rx_head + 1u) % RX_RING);
    if (nx != rx_tail) { rx_ring[rx_head] = c; rx_head = nx; }
    else rx_dropped++;
    if (c == '\r' || c == '\n') {
      uint8_t e = (uint8_t)((eol_head + 1u) & 3u);
      if (e != eol_tail) { eol_cyc[eol_head] = plat_cycles(); eol_head = e; }
      os_notify_isr(console_waiter);      /* ISR лише будить задачу cmd (методичка §4.10) */
    }
  }
}

bool plat_console_getline(char *buf, size_t size, uint32_t *t_cyc)
{
  static size_t len;
  while (rx_tail != rx_head) {
    char c = (char)rx_ring[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) % RX_RING);
    if (c == '\r' || c == '\n') {
      uint32_t t = 0;
      if (eol_tail != eol_head) { t = eol_cyc[eol_tail]; eol_tail = (uint8_t)((eol_tail + 1u) & 3u); }
      if (len == 0) continue;             /* \n після \r або порожній рядок */
      buf[len] = '\0';
      len = 0;
      if (t_cyc) *t_cyc = t;
      return true;
    }
    if (len + 1 < size) buf[len++] = c;
  }
  return false;
}

uint32_t plat_console_rx_dropped(void) { return rx_dropped; }
uint32_t plat_console_rx_overrun(void) { return rx_ore; }
uint32_t plat_console_rx_bytes(void)   { return rx_bytes; }

/* ================= GPIO ================= */
static const uint16_t probe_pin[3] = { BOARD_PROBE0_Pin, BOARD_PROBE1_Pin, BOARD_PROBE2_Pin };

void plat_probe(unsigned n, bool on)
{
  if (n < 3)       BOARD_PROBE_Port->BSRR = on ? probe_pin[n] : (uint32_t)probe_pin[n] << 16;
  else if (n == 3) BOARD_MARK_Port->BSRR  = on ? BOARD_MARK_Pin : (uint32_t)BOARD_MARK_Pin << 16;
}

void plat_mark(bool level) { plat_probe(PROBE_MARK, level); }
void plat_led_toggle(void) { BOARD_LED_Port->ODR ^= BOARD_LED_Pin; }

static volatile uint32_t btn_edges, btn_last_edge, btn_first_edge;
int      plat_btn_level(void)          { return (BOARD_BTN_Port->IDR & BOARD_BTN_Pin) ? 1 : 0; }
uint32_t plat_btn_edges(void)          { return btn_edges; }
uint32_t plat_btn_first_edge_cyc(void) { return btn_first_edge; }

/* EXTI8 на обидва фронти: лічильник брязкоту + мітка ПЕРШОГО спаду після ≥ 60 мс
   тиші — фізичний початок натискання (для латентності «фронт → реакція»). */
void board_btn_exti(void)
{
  uint32_t now = plat_cycles();
  uint32_t fall = EXTI->FPR1 & BOARD_BTN_Pin, rise = EXTI->RPR1 & BOARD_BTN_Pin;
  EXTI->FPR1 = fall;
  EXTI->RPR1 = rise;
  if (!(fall | rise)) return;
  btn_edges++;
  if (fall && (now - btn_last_edge) > 60u * (BOARD_SYSCLK_HZ / 1000u)) btn_first_edge = now;
  btn_last_edge = now;
}

/* ================= PWM (TIM3, 1 тік = 1 мкс) ================= */
void plat_servo_set_us(unsigned ch, uint16_t us)
{
  if (ch) BOARD_PWM_TIM->CCR2 = us; else BOARD_PWM_TIM->CCR1 = us;
}

uint16_t plat_servo_get_us(unsigned ch)
{
  return (uint16_t)(ch ? BOARD_PWM_TIM->CCR2 : BOARD_PWM_TIM->CCR1);
}

/* ================= АЦП (ADC1, одиночне перетворення каналу потенціометра) ================= */
int board_adc_calibrate(void)
{
  uint32_t t0 = HAL_GetTick();
  if (ADC1->CR & ADC_CR_ADEN) {                     /* ADEN знімається лише через ADDIS (RM0490) */
    if (ADC1->CR & ADC_CR_ADSTART) ADC1->CR |= ADC_CR_ADSTP;
    while (ADC1->CR & ADC_CR_ADSTP) if (HAL_GetTick() - t0 > 10u) return -1;
    ADC1->CR |= ADC_CR_ADDIS;
    while (ADC1->CR & ADC_CR_ADEN) if (HAL_GetTick() - t0 > 10u) return -1;
  }
  ADC1->CR |= ADC_CR_ADCAL;
  while (ADC1->CR & ADC_CR_ADCAL) if (HAL_GetTick() - t0 > 10u) return -1;
  ADC1->ISR = ADC_ISR_ADRDY;                         /* скинути старий прапорець */
  ADC1->CR |= ADC_CR_ADEN;
  t0 = HAL_GetTick();
  while (!(ADC1->ISR & ADC_ISR_ADRDY)) if (HAL_GetTick() - t0 > 10u) return -1;
  return 0;
}

int plat_pot_read_raw(void)
{
  uint32_t t0 = HAL_GetTick();
  ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
  ADC1->CR |= ADC_CR_ADSTART;
  while (!(ADC1->ISR & ADC_ISR_EOC)) if (HAL_GetTick() - t0 > 2u) return -1;
  return (int)(ADC1->DR & 0x0FFFu);
}

/* ================= пам'ять: stack painting (методичка ЛР3 §5.4) ================= */
extern uint32_t _estack, _Min_Stack_Size, _sdata, _ebss;
#define STACK_PATTERN 0xDEADBEEFu

static uint32_t *stack_bottom(void)
{
  return (uint32_t *)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);
}

void plat_stack_paint(void)
{
  uint32_t *p = stack_bottom();
  uint32_t *sp = (uint32_t *)__get_MSP();
  while (p < sp - 16) *p++ = STACK_PATTERN;
}

uint32_t plat_stack_used(void)
{
  uint32_t *p = stack_bottom();
  while (p < &_estack && *p == STACK_PATTERN) p++;
  return (uint32_t)&_estack - (uint32_t)p;
}

uint32_t plat_stack_size(void) { return (uint32_t)&_Min_Stack_Size; }
uint32_t plat_ram_static(void) { return (uint32_t)&_ebss - (uint32_t)&_sdata; }

/* ================= система ================= */
void plat_reset(void)
{
  while (!(BOARD_UART->ISR & USART_ISR_TC)) { }   /* договорити рядок */
  NVIC_SystemReset();
}

const char *plat_board_name(void) { return BOARD_NAME; }
uint32_t    plat_sysclk_hz(void)  { return BOARD_SYSCLK_HZ; }
uint32_t    plat_reset_cause(void) { return RCC->CSR2 >> 24; }
