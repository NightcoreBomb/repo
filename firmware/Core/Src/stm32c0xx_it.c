#include "main.h"
#include "stm32c0xx_it.h"
#include "app.h"
#include "FreeRTOS.h"
#include "task.h"

void NMI_Handler(void)       { while (1) { } }
void HardFault_Handler(void) { plat_log_raw("FATAL hardfault\r\n"); while (1) { } }

/* ЛР4: SVC_Handler — у порті FreeRTOS (portasm.c); SysTick_Handler і PendSV_Handler —
   у Core/Src/os_simfix.c (обхід П-29: Wokwi не маскує їх PRIMASK-ом). */

/* Tick hook (configUSE_TICK_HOOK): час HAL (uwTick += мс на тік) і debounce кнопки.
   Виконується в SysTick ISR — лише FromISR-виклики (app_tick_1ms будить задачу btn). */
#ifdef LR4_DIAG
void os_diag_tick(void);
#endif
void plat_console_tick(void);
void vApplicationTickHook(void)
{
#ifdef LR4_DIAG
  os_diag_tick();
#endif
  HAL_IncTick();
  app_tick_1ms();
  plat_console_tick();                    /* П-29: відкладене пробудження cmd з RX ISR */
}

/* PA8 кнопка → EXTI line 8 (обробник — Platform/plat_stm32.c) */
void EXTI4_15_IRQHandler(void) { board_btn_exti(); }

/* USART2: прийом команд консолі (обробник — Platform/plat_stm32.c) */
void USART2_IRQHandler(void)   { board_uart_rx_irq(); }
