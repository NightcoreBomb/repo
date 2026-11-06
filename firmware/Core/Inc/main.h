/* USER CODE BEGIN Header */
/**
 * main.h — ЛР2, NUCLEO-C031C6 (Wokwi). Структура файлу — як у STM32CubeMX:
 * власний код лише між USER CODE BEGIN/END. Піни і хендли — у Platform/board.h.
 */
/* USER CODE END Header */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32c0xx_hal.h"

/* USER CODE BEGIN Includes */
#include "board.h"
/* USER CODE END Includes */

void Error_Handler(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
