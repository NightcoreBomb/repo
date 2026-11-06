#include "main.h"
#include "stm32c0xx_it.h"
#include "app.h"

void NMI_Handler(void)       { while (1) { } }
void HardFault_Handler(void) { while (1) { } }
void SVC_Handler(void)       { }
void PendSV_Handler(void)    { }
/* 1 кГц: системний час + debounce кнопки (app_tick_1ms — кілька тактів, без логування) */
void SysTick_Handler(void)   { HAL_IncTick(); app_tick_1ms(); }

/* PA8 кнопка → EXTI line 8 */
void EXTI4_15_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(BOARD_BTN_Pin); }

/* USART2: прийом команд консолі (обробник — Platform/plat_stm32.c) */
void USART2_IRQHandler(void)   { board_uart_rx_irq(); }
