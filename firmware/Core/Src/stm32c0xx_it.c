#include "main.h"
#include "stm32c0xx_it.h"
#include "app.h"
#include "FreeRTOS.h"
#include "task.h"

void NMI_Handler(void)       { while (1) { } }
void HardFault_Handler(void) { plat_log_raw("FATAL hardfault\r\n"); while (1) { } }

/* ЛР4: SVC_Handler і PendSV_Handler — у порті FreeRTOS (portasm.c).
   SysTick належить ядру; обробник порту перейменовано на xPortSysTickHandler (Makefile),
   щоб до старту планувальника вести час HAL (HAL_Delay в ініціалізації) і щоб виміряти
   вартість тіку. */
void xPortSysTickHandler(void);

void SysTick_Handler(void)
{
  if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
    HAL_IncTick();
    app_tick_1ms();
    return;
  }
  uint32_t v0 = SysTick->VAL;
  xPortSysTickHandler();
  os_tick_account(v0);
}

/* Tick hook (configUSE_TICK_HOOK): час HAL (uwTick += мс на тік) і debounce кнопки.
   Виконується в SysTick ISR — лише FromISR-виклики (app_tick_1ms будить задачу btn). */
void vApplicationTickHook(void)
{
  HAL_IncTick();
  app_tick_1ms();
}

/* PA8 кнопка → EXTI line 8 (обробник — Platform/plat_stm32.c) */
void EXTI4_15_IRQHandler(void) { board_btn_exti(); }

/* USART2: прийом команд консолі (обробник — Platform/plat_stm32.c) */
void USART2_IRQHandler(void)   { board_uart_rx_irq(); }
