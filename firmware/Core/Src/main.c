/* USER CODE BEGIN Header */
/**
 * main.c — ЛР2: інтеграція сенсорної підсистеми (Форма B, Wokwi)
 * Плата: NUCLEO-C031C6 (Cortex-M0+, 48 МГц, 32 КБ Flash, 12 КБ RAM)
 *
 * Шар Core: лише ініціалізація периферії (у стилі STM32CubeMX) і передача
 * керування прикладному шару. Уся логіка — в App/, драйвери — в Devices/,
 * доступ до заліза — через Platform/ (див. docs/architecture.md).
 */
/* USER CODE END Header */
#include "main.h"

/* USER CODE BEGIN Includes */
#include "plat.h"
#include "app.h"
#include "selftest.h"
/* USER CODE END Includes */

ADC_HandleTypeDef  hadc1;
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi1;
TIM_HandleTypeDef  htim3;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static app_t app;
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static HAL_StatusTypeDef MX_USART2_UART_Init(void);
static HAL_StatusTypeDef MX_I2C1_Init(void);
static HAL_StatusTypeDef MX_SPI1_Init(void);
static HAL_StatusTypeDef MX_ADC1_Init(void);
static HAL_StatusTypeDef MX_TIM3_Init(void);

/* USER CODE BEGIN 0 */
static void report(const char *name, HAL_StatusTypeDef s)
{
  plat_log("[init] %-14s %s\r\n", name, board_hal_str(s));
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  HAL_StatusTypeDef st_uart = MX_USART2_UART_Init();   /* UART перший — щоб бачити решту */

  /* USER CODE BEGIN 2 */
  plat_log("\r\n==== hello from STM32 ====\r\n");
  plat_log("LR2 sensor integration | %s | SYSCLK = %lu Hz\r\n", BOARD_NAME,
           (unsigned long)HAL_RCC_GetSysClockFreq());
  report("USART2", st_uart);
  report("I2C1",   MX_I2C1_Init());
  report("SPI1",   MX_SPI1_Init());
  report("ADC1",   MX_ADC1_Init());
  report("TIM3 PWM", MX_TIM3_Init());
  board_start();

  app_init(&app, &board_i2c1, &board_sd_spi);
  selftest_run(&app);
  plat_log("==== bring-up done, entering main loop (type 'help') ====\r\n");
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    app_poll(&app);
    /* USER CODE END 3 */
  }
}

/* HSI 48 МГц, без дільників: SYSCLK = HCLK = PCLK = 48 МГц (RM0490, RCC) */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState            = RCC_HSI_ON;
  osc.HSIDiv              = RCC_HSI_DIV1;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  clk.SYSCLKDivider  = RCC_SYSCLK_DIV1;
  clk.AHBCLKDivider  = RCC_HCLK_DIV1;
  clk.APB1CLKDivider = RCC_APB1_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

static HAL_StatusTypeDef MX_USART2_UART_Init(void)
{
  huart2.Instance                    = USART2;
  huart2.Init.BaudRate               = 115200;    /* BRR = 48e6/115200 = 416.7 → 417, похибка +0.08 % */
  huart2.Init.WordLength             = UART_WORDLENGTH_8B;
  huart2.Init.StopBits               = UART_STOPBITS_1;
  huart2.Init.Parity                 = UART_PARITY_NONE;
  huart2.Init.Mode                   = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  return HAL_UART_Init(&huart2);
}

static HAL_StatusTypeDef MX_I2C1_Init(void)
{
  HAL_StatusTypeDef s;
  hi2c1.Instance              = I2C1;
  hi2c1.Init.Timing           = BOARD_I2C_TIMING;   /* 100 кГц, розрахунок у board.h */
  hi2c1.Init.OwnAddress1      = 0;
  hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2      = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
  if ((s = HAL_I2C_Init(&hi2c1)) != HAL_OK) return s;
  if ((s = HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)) != HAL_OK) return s;
  return HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}

static HAL_StatusTypeDef MX_SPI1_Init(void)
{
  hspi1.Instance               = SPI1;
  hspi1.Init.Mode              = SPI_MODE_MASTER;
  hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;    /* SPI mode 0: CPOL=0 */
  hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;     /*             CPHA=0 */
  hspi1.Init.NSS               = SPI_NSS_SOFT;        /* CS# — GPIO PB0     */
  hspi1.Init.BaudRatePrescaler = BOARD_SPI_PRESC_SLOW;
  hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial     = 7;
  hspi1.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
  return HAL_SPI_Init(&hspi1);
}

static HAL_StatusTypeDef MX_ADC1_Init(void)
{
  HAL_StatusTypeDef s;
  ADC_ChannelConfTypeDef ch = {0};

  hadc1.Instance                   = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait      = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff  = DISABLE;
  hadc1.Init.ContinuousConvMode    = DISABLE;
  hadc1.Init.NbrOfConversion       = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_39CYCLES_5;
  hadc1.Init.OversamplingMode      = DISABLE;
  hadc1.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_HIGH;
  if ((s = HAL_ADC_Init(&hadc1)) != HAL_OK) return s;

  ch.Channel      = BOARD_POT_ADC_CH;
  ch.Rank         = ADC_REGULAR_RANK_1;
  ch.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  return HAL_ADC_ConfigChannel(&hadc1, &ch);
}

static HAL_StatusTypeDef MX_TIM3_Init(void)
{
  HAL_StatusTypeDef s;
  TIM_OC_InitTypeDef oc = {0};

  htim3.Instance               = TIM3;
  htim3.Init.Prescaler         = BOARD_TIM3_PSC;   /* 48 МГц / 48 = 1 МГц → 1 мкс */
  htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim3.Init.Period            = BOARD_TIM3_ARR;   /* 20 мс = 50 Гц */
  htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if ((s = HAL_TIM_PWM_Init(&htim3)) != HAL_OK) return s;

  oc.OCMode     = TIM_OCMODE_PWM1;
  oc.Pulse      = 1500;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  if ((s = HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_1)) != HAL_OK) return s;
  if ((s = HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_2)) != HAL_OK) return s;
  HAL_TIM_MspPostInit(&htim3);
  return HAL_OK;
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(BOARD_LED_Port,   BOARD_LED_Pin,   GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BOARD_SD_CS_Port, BOARD_SD_CS_Pin, GPIO_PIN_SET);    /* CS# неактивний */
  HAL_GPIO_WritePin(BOARD_MARK_Port,  BOARD_MARK_Pin,  GPIO_PIN_RESET);

  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Pin   = BOARD_LED_Pin;
  HAL_GPIO_Init(BOARD_LED_Port, &g);

  g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Pin   = BOARD_SD_CS_Pin;
  HAL_GPIO_Init(BOARD_SD_CS_Port, &g);
  g.Pin   = BOARD_MARK_Pin;
  HAL_GPIO_Init(BOARD_MARK_Port, &g);

  /* Кнопка: замикає на GND, внутрішній pull-up, переривання по спаду */
  g.Pin  = BOARD_BTN_Pin;
  g.Mode = GPIO_MODE_IT_FALLING;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BOARD_BTN_Port, &g);

  HAL_NVIC_SetPriority(BOARD_BTN_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(BOARD_BTN_IRQn);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    HAL_GPIO_TogglePin(BOARD_LED_Port, BOARD_LED_Pin);   /* швидке мигання = фатальна помилка */
    for (volatile uint32_t i = 0; i < 200000; i++) { }
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  plat_log("assert failed: %s:%lu\r\n", file, (unsigned long)line);
}
#endif
