/**
 * os_simfix.c — обробники SysTick і PendSV для FreeRTOS у Wokwi (журнал ЛР4, П-29).
 *
 * Wokwi (модель STM32C031 / Cortex-M0+) НЕ маскує SysTick і PendSV інструкцією `cpsid i`
 * (тест PRIMASK_TEST / CRIT_TEST: 405 тіків і 318 перемикань контексту всередині
 * taskENTER_CRITICAL). На ARMv6-M критична секція FreeRTOS — це саме PRIMASK, тож тік і
 * перемикання задач посеред операції над списками ядра їх ламали (П-27).
 *
 * Обхід: обидва обробники перевіряють лічильник вкладеності критичних секцій порту
 * (ulCriticalNesting). Якщо ядро в секції — тік/перемикання відкладаються, а
 * vPortExitCritical (порт, LR4-патч) знову виставляє їх, щойно секцію закрито. На
 * справжньому МК PRIMASK працює, ці гілки ніколи не виконуються — обхід нешкідливий.
 *
 * Усі переривання, що викликають API ядра (SysTick, PendSV, USART2), мають однаковий
 * пріоритет (3), тож одне одного не витісняють. Файл компілюється без LTO (naked-асемблер).
 */
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app.h"

extern volatile uint32_t ulCriticalNesting;          /* port.c */
void xPortSysTickHandler(void);                      /* port.c, перейменований SysTick_Handler */
void xPortPendSVHandler(void);                       /* portasm.c, перейменований PendSV_Handler */
void os_tick_account(uint32_t v0);
void plat_console_tick(void);

volatile uint32_t os_pendsv_deferred;
static volatile uint32_t ticks_owed;
volatile uint32_t os_sim_deferred_ticks, os_sim_deferred_switches;   /* статистика для `tasks` */

void SysTick_Handler(void)
{
  if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {   /* час HAL до старту ОС */
    HAL_IncTick();
    app_tick_1ms();
    return;
  }
  if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) ticks_owed++;  /* справжній тік, не PENDSTSET */
  if (ulCriticalNesting != 0u) { os_sim_deferred_ticks++; return; }
  while (ticks_owed) {
    ticks_owed--;
    uint32_t v0 = SysTick->VAL;
    xPortSysTickHandler();
    os_tick_account(v0);
  }
}

/* Викликається портом, коли критична секція закрилась (контекст задачі, переривання дозволені) */
void vPortSimCriticalExit(void)
{
  if (ticks_owed) SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
  if (os_pendsv_deferred) {
    os_pendsv_deferred = 0;
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
  }
}

void PendSV_Handler(void) __attribute__((naked));
void PendSV_Handler(void)
{
  __asm volatile(
    " .syntax unified                 \n"
    " ldr  r0, =ulCriticalNesting     \n"
    " ldr  r0, [r0]                   \n"
    " cmp  r0, #0                     \n"
    " bne  1f                         \n"
    " ldr  r0, =xPortPendSVHandler    \n"   /* звичайний шлях: перемикання порту */
    " bx   r0                         \n"
    "1:                               \n"   /* ядро в критичній секції: відкласти */
    " ldr  r0, =os_pendsv_deferred    \n"
    " movs r1, #1                     \n"
    " str  r1, [r0]                   \n"
    " ldr  r0, =os_sim_deferred_switches \n"
    " ldr  r1, [r0]                   \n"
    " adds r1, r1, #1                 \n"
    " str  r1, [r0]                   \n"
    " bx   lr                         \n"
    " .ltorg                          \n");
}
