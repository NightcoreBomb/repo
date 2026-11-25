/* USER CODE BEGIN Header */
/**
 * main.c — ЛР3: продукт Motion-Mirroring Teleoperator на bare metal (super loop)
 * Плата: NUCLEO-C031C6 (Cortex-M0+, 48 МГц, 32 КБ Flash, 12 КБ RAM), Форма B (Wokwi)
 *
 * Шар Core: ініціалізація тактування й периферії (структура як у STM32CubeMX, тіла MX_*
 * — у стилі LL, регістри напряму: HAL_I2C/SPI/ADC/TIM/UART не вміщалися у 32 КБ Flash,
 * журнал ЛР3 П-13) і передача керування прикладному шару. Головний цикл — App/app.c.
 */
/* USER CODE END Header */
#include "main.h"
#include "FreeRTOS.h"

/* USER CODE BEGIN Includes */
#include "plat.h"
#include "app.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
static app_t app;
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static int  MX_ADC1_Init(void);
static void MX_TIM3_Init(void);

static void gpio_af(GPIO_TypeDef *port, uint32_t pins, uint32_t mode, uint32_t pull,
                    uint32_t speed, uint32_t af)
{
  GPIO_InitTypeDef g = { 0 };
  g.Pin = pins; g.Mode = mode; g.Pull = pull; g.Speed = speed; g.Alternate = af;
  HAL_GPIO_Init(port, &g);
}

int main(void)
{
  /* USER CODE BEGIN 1 */
  plat_stack_paint();             /* до будь-яких викликів: шаблон для виміру глибини стека */
  /* USER CODE END 1 */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();          /* UART перший — щоб бачити решту */
  MX_I2C1_Init();
  MX_SPI1_Init();
  int adc = MX_ADC1_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  app_init(&app, &board_i2c1, &board_sd_spi);
  app_start(&app, adc == 0);      /* банер, FR-12 (перевірки) — ще без планувальника, HAL_Delay */
  /* ЛР4: далі HAL-час веде tick hook FreeRTOS: HAL_IncTick додає uwTickFreq мс на тік ядра */
  uwTickFreq = (HAL_TickFreqTypeDef)(1000u / configTICK_RATE_HZ);
  app_rtos_start(&app);           /* м'ютекси, черги, задачі + vTaskStartScheduler(); не повертається */
  /* USER CODE END 2 */
}

/* HSI 48 МГц, без дільників: SYSCLK = HCLK = PCLK = 48 МГц (RM0490, RCC) */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = { 0 };
  RCC_ClkInitTypeDef clk = { 0 };

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

/* USART2 → VCP, 115200 8N1: BRR = 417 (−0.08 %), переривання на прийом (консоль) */
static void MX_USART2_UART_Init(void)
{
  __HAL_RCC_USART2_CLK_ENABLE();
  gpio_af(BOARD_VCP_TX_Port, BOARD_VCP_TX_Pin | BOARD_VCP_RX_Pin, GPIO_MODE_AF_PP,
          GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, BOARD_VCP_AF);
  BOARD_UART->BRR = BOARD_UART_BRR;
  BOARD_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE;
  HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);   /* П-29: як SysTick/PendSV — без вкладення ISR */
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/* I2C1 100 кГц (TIMINGR — board.h), open-drain + pull-up (UM10204 §3.1.1) */
static void MX_I2C1_Init(void)
{
  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C1SEL, 0u);             /* I2C1CLK = PCLK */
  gpio_af(BOARD_I2C_SCL_Port, BOARD_I2C_SCL_Pin | BOARD_I2C_SDA_Pin, GPIO_MODE_AF_OD,
          GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, BOARD_I2C_AF);
  __HAL_RCC_I2C1_CLK_ENABLE();
  BOARD_I2C->CR1 = 0;                                        /* PE=0, аналоговий фільтр ON, DNF=0 */
  BOARD_I2C->TIMINGR = BOARD_I2C_TIMING;
  BOARD_I2C->CR1 |= I2C_CR1_PE;
}

/* SPI1 master, mode 0, 8 біт, CS# програмний (PB0), старт на 375 кГц */
static void MX_SPI1_Init(void)
{
  __HAL_RCC_SPI1_CLK_ENABLE();
  gpio_af(BOARD_SPI_SCK_Port, BOARD_SPI_SCK_Pin | BOARD_SPI_MISO_Pin | BOARD_SPI_MOSI_Pin,
          GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, BOARD_SPI_AF);
  BOARD_SPI->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | BOARD_SPI_BR_SLOW;
  BOARD_SPI->CR2 = (7u << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;   /* 8 біт, RXNE на 8 біт */
  BOARD_SPI->CR1 |= SPI_CR1_SPE;
}

/* ADC1: f_ADC = PCLK/2 = 24 МГц, 12 біт, IN0 (PA0), 39.5 циклу вибірки → 2.17 мкс */
static int MX_ADC1_Init(void)
{
  GPIO_InitTypeDef g = { 0 };
  g.Pin = BOARD_POT_Pin; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOARD_POT_Port, &g);
  __HAL_RCC_ADC_CLK_ENABLE();
  /* ЛР3 П-16: після програмного скиду (команда reset) Wokwi не повертає АЦП у стан після скиду:
   * ADEN лишався 1, калібрування не стартувало → adc=FAIL. Явний скид блоку через RCC. */
  __HAL_RCC_ADC_FORCE_RESET();
  __HAL_RCC_ADC_RELEASE_RESET();
  ADC1->CR |= ADC_CR_ADVREGEN;                               /* регулятор: tADCVREG_STUP ≤ 20 мкс */
  HAL_Delay(1);
  ADC1->CFGR2 = (1u << ADC_CFGR2_CKMODE_Pos);                /* PCLK/2 */
  ADC1->CFGR1 = 0;                                           /* 12 біт, вирівнювання вправо, одиночне */
  ADC1->SMPR  = (5u << ADC_SMPR_SMP1_Pos);                   /* 39.5 циклу */
  if (board_adc_calibrate() != 0) return -1;
  ADC1->CHSELR = 1u << BOARD_POT_ADC_CH;
  uint32_t t0 = HAL_GetTick();
  while (!(ADC1->ISR & ADC_ISR_CCRDY)) if (HAL_GetTick() - t0 > 5u) return -1;
  ADC1->ISR = ADC_ISR_CCRDY;
  return 0;
}

/* TIM3: 1 МГц (PSC=47), 50 Гц (ARR=19999), CH1/CH2 PWM1 з preload, 1500 мкс (центр) */
static void MX_TIM3_Init(void)
{
  __HAL_RCC_TIM3_CLK_ENABLE();
  gpio_af(BOARD_SERVO_Port, BOARD_SERVO_PAN_Pin | BOARD_SERVO_TILT_Pin, GPIO_MODE_AF_PP,
          GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, BOARD_SERVO_AF);
  TIM_TypeDef *T = BOARD_PWM_TIM;
  T->PSC   = BOARD_TIM3_PSC;
  T->ARR   = BOARD_TIM3_ARR;
  T->CCR1  = 1500;
  T->CCR2  = 1500;
  T->CCMR1 = (6u << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE |
             (6u << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;   /* PWM mode 1 */
  T->CCER  = TIM_CCER_CC1E | TIM_CCER_CC2E;
  T->CR1   = TIM_CR1_ARPE;
  T->EGR   = TIM_EGR_UG;                                     /* завантажити PSC/ARR/CCR */
  T->CR1  |= TIM_CR1_CEN;
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef g = { 0 };

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(BOARD_LED_Port,   BOARD_LED_Pin,   GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BOARD_SD_CS_Port, BOARD_SD_CS_Pin, GPIO_PIN_SET);    /* CS# неактивний */
  HAL_GPIO_WritePin(BOARD_MARK_Port,  BOARD_MARK_Pin,  GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BOARD_PROBE_Port, BOARD_PROBE0_Pin | BOARD_PROBE1_Pin | BOARD_PROBE2_Pin,
                    GPIO_PIN_RESET);

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
  g.Pin   = BOARD_PROBE0_Pin | BOARD_PROBE1_Pin | BOARD_PROBE2_Pin;   /* службові GPIO ЛР3 */
  HAL_GPIO_Init(BOARD_PROBE_Port, &g);

  /* Кнопка: на GND, внутрішній pull-up; EXTI на обидва фронти — лічильник брязкоту і
     мітка першого спаду; рішення «натиснуто» — debounce 1 кГц у SysTick (П-12) */
  g.Pin  = BOARD_BTN_Pin;
  g.Mode = GPIO_MODE_IT_RISING_FALLING;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BOARD_BTN_Port, &g);

  HAL_NVIC_SetPriority(BOARD_BTN_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(BOARD_BTN_IRQn);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    BOARD_LED_Port->ODR ^= BOARD_LED_Pin;   /* швидке мигання = фатальна помилка */
    for (volatile uint32_t i = 0; i < 200000; i++) { }
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  plat_log("assert failed: %s:%lu\r\n", file, (unsigned long)line);
}
#endif
