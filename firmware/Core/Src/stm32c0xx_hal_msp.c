/**
 * MSP (MCU Support Package). ЛР3: периферія ініціалізується в Core/Src/main.c напряму
 * (регістри, П-13), тож тут лишилось тактування системних блоків.
 */
#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}
