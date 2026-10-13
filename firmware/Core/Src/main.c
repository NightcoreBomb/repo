/**
 * Лабораторна робота №1 — bring-up STM32-платформи (Форма B, Wokwi)
 * Плата: NUCLEO-C031C6 (Cortex-M0+, 48 МГц, 32 КБ Flash, 12 КБ RAM)
 *
 * Що робить прошивка:
 *   1. Ініціалізує HAL, тактування (HSI 48 МГц) і всю периферію,
 *      друкуючи результат (HAL_OK / ERROR) кожної ініціалізації в UART.
 *   2. Сканує шину I2C1 (HAL_I2C_IsDeviceReady) — має знайти 0x3C та 0x68.
 *   3. Читає WHO_AM_I MPU-6050 (регістр 0x75, очікується 0x68).
 *   4. Шле «пробні» такти по SPI1 (CS=low, 10 x 0xFF) — видно на логічному аналізаторі.
 *   5. Ставить обидва серво (TIM3 PWM 50 Гц) в центр — 1500 мкс.
 *   6. Щосекунди: "tick N | adc=... | btn=..." + мигає LED.
 *   7. Натискання кнопки (EXTI) друкується одразу.
 *
 * Драйверів сенсорів тут ще немає — тільки ініціалізація інтерфейсів (крок 6 методички).
 */
#include "main.h"
#include <stdio.h>
#include <string.h>

ADC_HandleTypeDef  hadc1;
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi1;
TIM_HandleTypeDef  htim3;
UART_HandleTypeDef huart2;

#define MPU6050_ADDR      (0x68u << 1)
#define MPU6050_WHO_AM_I  0x75u
#define SERVO_CENTER_US   1500u

static volatile uint32_t btn_presses = 0;
static volatile uint8_t  btn_event   = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static HAL_StatusTypeDef MX_USART2_UART_Init(void);
static HAL_StatusTypeDef MX_I2C1_Init(void);
static HAL_StatusTypeDef MX_SPI1_Init(void);
static HAL_StatusTypeDef MX_ADC1_Init(void);
static HAL_StatusTypeDef MX_TIM3_Init(void);

/* ---------- printf -> USART2 (newlib _write hook) ---------- */
int _write(int fd, char *p, int n)
{
  (void)fd;
  HAL_UART_Transmit(&huart2, (uint8_t *)p, (uint16_t)n, HAL_MAX_DELAY);
  return n;
}

static const char *st2str(HAL_StatusTypeDef s)
{
  switch (s) {
    case HAL_OK:      return "HAL_OK";
    case HAL_ERROR:   return "HAL_ERROR";
    case HAL_BUSY:    return "HAL_BUSY";
    case HAL_TIMEOUT: return "HAL_TIMEOUT";
    default:          return "?";
  }
}

static void report(const char *name, HAL_StatusTypeDef s)
{
  printf("[init] %-14s %s\r\n", name, st2str(s));
}

static void i2c_scan(void)
{
  uint8_t found = 0;
  printf("[i2c ] scan 0x08..0x77:");
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 1, 5) == HAL_OK) {
      printf(" 0x%02X", a);
      found++;
    }
  }
  printf("  (%u device(s))\r\n", found);
}

static void mpu6050_probe(void)
{
  uint8_t reg = MPU6050_WHO_AM_I;
  uint8_t who = 0;
  HAL_StatusTypeDef s;

  // Коротка затримка після сканера I2C
  HAL_Delay(10);

  // 1. Надсилаємо адресу регістра (0x75)
  s = HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDR, &reg, 1, 100);
  if (s == HAL_OK) {
    // 2. Читаємо значення з регістра
    s = HAL_I2C_Master_Receive(&hi2c1, MPU6050_ADDR, &who, 1, 100);
  }

  printf("[i2c ] MPU-6050 WHO_AM_I = 0x%02X (%s, expected 0x68)\r\n", who, st2str(s));
}

static void spi_probe(void)
{
  uint8_t ff[10];
  memset(ff, 0xFF, sizeof ff);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
  HAL_StatusTypeDef s = HAL_SPI_Transmit(&hspi1, ff, sizeof ff, 50);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  printf("[spi ] 80 probe clocks on SCK: %s\r\n", st2str(s));
}

static uint32_t adc_read_raw(void)
{
  uint32_t v = 0;
  if (HAL_ADC_Start(&hadc1) == HAL_OK) {
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
      v = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
  }
  return v;
}

int main(void)
{
  HAL_StatusTypeDef st;

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  st = MX_USART2_UART_Init();           /* UART перший — щоб бачити решту */
  printf("\r\n==== hello from STM32 ====\r\n");
  printf("LR1 bring-up | %s | SYSCLK = %lu Hz\r\n", "NUCLEO-C031C6", HAL_RCC_GetSysClockFreq());
  report("USART2", st);
  report("I2C1",   MX_I2C1_Init());
  report("SPI1",   MX_SPI1_Init());
  report("ADC1",   MX_ADC1_Init());
  report("ADC1 calib", HAL_ADCEx_Calibration_Start(&hadc1));
  report("TIM3 PWM", MX_TIM3_Init());

  report("PWM CH1 start", HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1));
  report("PWM CH2 start", HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2));
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, SERVO_CENTER_US);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, SERVO_CENTER_US);

  i2c_scan();
  mpu6050_probe();
  spi_probe();
  printf("==== bring-up done, entering main loop ====\r\n");

  uint32_t next = HAL_GetTick() + 1000;
  uint32_t n = 0;
  while (1)
  {
    if (btn_event) {
      btn_event = 0;
      printf("[btn ] pressed, total = %lu\r\n", btn_presses);
    }
    if ((int32_t)(HAL_GetTick() - next) >= 0) {
      next += 1000;
      n++;
      uint32_t raw = adc_read_raw();
      uint32_t mv  = raw * 3300u / 4095u;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      printf("tick %lu | t=%lu ms | adc=%4lu (%4lu mV) | btn=%lu\r\n",
             n, HAL_GetTick(), raw, mv, btn_presses);
    }
  }
}

/* ---------- кнопка: EXTI по спаду, простий debounce 50 мс ---------- */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last = 0;
  if (GPIO_Pin == BTN_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - last) > 150) {
      btn_presses++;
      btn_event = 1;
      last = now; // Оновлюємо тільки при зарахованому натисканні
    }
  }
}

/* ---------- тактування: HSI 48 МГц, без дільників ---------- */
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
  huart2.Init.BaudRate               = 115200;
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
  hi2c1.Init.Timing           = 0x20303E5D;   /* 100 кГц @ PCLK 48 МГц (значення CubeMX) */
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
  hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;    /* SPI mode 0 */
  hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi1.Init.NSS               = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; /* 375 кГц — < 400 кГц для init SD */
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

  ch.Channel      = ADC_CHANNEL_0;          /* PA0 */
  ch.Rank         = ADC_REGULAR_RANK_1;
  ch.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  return HAL_ADC_ConfigChannel(&hadc1, &ch);
}

static HAL_StatusTypeDef MX_TIM3_Init(void)
{
  HAL_StatusTypeDef s;
  TIM_OC_InitTypeDef oc = {0};

  htim3.Instance               = TIM3;
  htim3.Init.Prescaler         = 48 - 1;      /* 48 МГц / 48 = 1 МГц -> 1 тік = 1 мкс */
  htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim3.Init.Period            = 20000 - 1;   /* 20 мс = 50 Гц, стандарт для серво */
  htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if ((s = HAL_TIM_PWM_Init(&htim3)) != HAL_OK) return s;

  oc.OCMode     = TIM_OCMODE_PWM1;
  oc.Pulse      = SERVO_CENTER_US;
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

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  /* LED LD4 */
  g.Pin   = LED_Pin;
  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &g);

  /* SD chip-select */
  g.Pin   = SD_CS_Pin;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &g);

  /* Кнопка: замикає на GND, внутрішній pull-up, переривання по спаду */
  g.Pin  = BTN_Pin;
  g.Mode = GPIO_MODE_IT_FALLING;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_GPIO_Port, &g);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    /* швидке мигання LED = фатальна помилка */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    for (volatile uint32_t i = 0; i < 200000; i++) { }
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  printf("assert failed: %s:%lu\r\n", file, line);
}
#endif
