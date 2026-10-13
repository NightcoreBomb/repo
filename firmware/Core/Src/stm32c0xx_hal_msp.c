/**
 * MSP (MCU Support Package): тактування периферії + alternate functions пінів.
 * Саме тут видно, який пін за який інтерфейс відповідає (див. datasheet STM32C031, табл. AF).
 */
#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef g = {0};
  if (huart->Instance == USART2) {
    /* На STM32C031 USART2 тактується тільки від PCLK — вибору джерела немає */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA2 -> USART2_TX, PA3 -> USART2_RX (AF1) */
    g.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(GPIOA, &g);
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef g = {0};
  RCC_PeriphCLKInitTypeDef pclk = {0};
  if (hi2c->Instance == I2C1) {
    pclk.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    pclk.I2c1ClockSelection   = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) Error_Handler();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /* PB8 -> I2C1_SCL, PB9 -> I2C1_SDA (AF6), open-drain */
    g.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    g.Mode      = GPIO_MODE_AF_OD;
    g.Pull      = GPIO_PULLUP;   /* на залізі — ще й зовнішні 4.7 кОм (на GY-521 вже є) */
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF6_I2C1;
    HAL_GPIO_Init(GPIOB, &g);
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef g = {0};
  if (hspi->Instance == SPI1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA1 -> SPI1_SCK, PA6 -> SPI1_MISO, PA7 -> SPI1_MOSI (AF0).
       SCK винесено з PA5 на PA1, бо PA5 зайнятий LED LD4. */
    g.Pin       = GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_7;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(GPIOA, &g);
  }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  GPIO_InitTypeDef g = {0};
  RCC_PeriphCLKInitTypeDef pclk = {0};
  if (hadc->Instance == ADC1) {
    pclk.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    pclk.AdcClockSelection    = RCC_ADCCLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) Error_Handler();
    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA0 -> ADC1_IN0 */
    g.Pin  = GPIO_PIN_0;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
  }
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
  }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef g = {0};
  if (htim->Instance == TIM3) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /* PB4 -> TIM3_CH1 (pan), PB5 -> TIM3_CH2 (tilt), AF1 */
    g.Pin       = GPIO_PIN_4 | GPIO_PIN_5;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM3;
    HAL_GPIO_Init(GPIOB, &g);
  }
}
