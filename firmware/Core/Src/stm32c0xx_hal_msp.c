/**
 * MSP (MCU Support Package): тактування периферії + alternate functions пінів.
 * Номери пінів і AF беруться з Platform/board.h (єдине джерело, див. pinout.md).
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
    g.Pin       = BOARD_VCP_TX_Pin | BOARD_VCP_RX_Pin;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = BOARD_VCP_AF;
    HAL_GPIO_Init(BOARD_VCP_TX_Port, &g);
    HAL_NVIC_SetPriority(USART2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
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
    /* Open-drain: на шині кілька пристроїв, «1» = відпущена лінія (UM10204 §3.1.1) */
    g.Pin       = BOARD_I2C_SCL_Pin | BOARD_I2C_SDA_Pin;
    g.Mode      = GPIO_MODE_AF_OD;
    g.Pull      = GPIO_PULLUP;   /* ~40 кОм лише як «страховка»; основні — 4.7 кОм на модулях */
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = BOARD_I2C_AF;
    HAL_GPIO_Init(BOARD_I2C_SCL_Port, &g);
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef g = {0};
  if (hspi->Instance == SPI1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin       = BOARD_SPI_SCK_Pin | BOARD_SPI_MISO_Pin | BOARD_SPI_MOSI_Pin;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = BOARD_SPI_AF;
    HAL_GPIO_Init(BOARD_SPI_SCK_Port, &g);
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
    g.Pin  = BOARD_POT_Pin;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BOARD_POT_Port, &g);
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
    g.Pin       = BOARD_SERVO_PAN_Pin | BOARD_SERVO_TILT_Pin;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = BOARD_SERVO_AF;
    HAL_GPIO_Init(BOARD_SERVO_Port, &g);
  }
}
