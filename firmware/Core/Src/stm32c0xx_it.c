#include "main.h"
#include "stm32c0xx_it.h"

void NMI_Handler(void)       { while (1) { } }
void HardFault_Handler(void) { while (1) { } }
void SVC_Handler(void)       { }
void PendSV_Handler(void)    { }
void SysTick_Handler(void)   { HAL_IncTick(); }

/* PA8 кнопка -> EXTI line 8 */
void EXTI4_15_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(BTN_Pin); }
