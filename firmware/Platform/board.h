/**
 * board.h — ЄДИНЕ місце знання про плату NUCLEO-C031C6 + стенд ЛР2.
 *
 * Кожен рядок тут відповідає рядку таблиці docs/pinout.md і з'єднанню в
 * diagram.json; узгодженість трьох джерел перевіряє tools/check_pinout.py
 * (make check). Сюди не можуть включати App/ і Devices/ (tools/check_deps.py).
 *
 * Документи: DS13867 (datasheet STM32C031x4/x6) — табл. «Port A/B alternate
 * function mapping»; RM0490 (reference manual STM32C0x1); UM2953 (Nucleo-C031C6).
 */
#ifndef PLATFORM_BOARD_H
#define PLATFORM_BOARD_H

#include "stm32c0xx_hal.h"
#include "bus_i2c.h"
#include "bus_spi.h"

#define BOARD_NAME            "NUCLEO-C031C6"

/* ---------- піни: <РОЛЬ>_Port / _Pin / _AF  ---------- */
/* USART2 → ST-LINK VCP (UM2953 «Virtual COM port»): PA2/PA3, AF1          */
#define BOARD_VCP_TX_Port      GPIOA
#define BOARD_VCP_TX_Pin       GPIO_PIN_2
#define BOARD_VCP_RX_Port      GPIOA
#define BOARD_VCP_RX_Pin       GPIO_PIN_3
#define BOARD_VCP_AF           GPIO_AF1_USART2
/* I2C1: PB8 SCL / PB9 SDA, AF6 — Arduino D15/D14 (CN5)                     */
#define BOARD_I2C_SCL_Port     GPIOB
#define BOARD_I2C_SCL_Pin      GPIO_PIN_8
#define BOARD_I2C_SDA_Port     GPIOB
#define BOARD_I2C_SDA_Pin      GPIO_PIN_9
#define BOARD_I2C_AF           GPIO_AF6_I2C1
/* SPI1: PA1 SCK (не PA5 — там LD4) / PA6 MISO / PA7 MOSI, AF0              */
#define BOARD_SPI_SCK_Port     GPIOA
#define BOARD_SPI_SCK_Pin      GPIO_PIN_1
#define BOARD_SPI_MISO_Port    GPIOA
#define BOARD_SPI_MISO_Pin     GPIO_PIN_6
#define BOARD_SPI_MOSI_Port    GPIOA
#define BOARD_SPI_MOSI_Pin     GPIO_PIN_7
#define BOARD_SPI_AF           GPIO_AF0_SPI1
#define BOARD_SD_CS_Port       GPIOB           /* програмний CS#, D10 */
#define BOARD_SD_CS_Pin        GPIO_PIN_0
/* ADC1_IN0 — потенціометр, A0                                               */
#define BOARD_POT_Port         GPIOA
#define BOARD_POT_Pin          GPIO_PIN_0
#define BOARD_POT_ADC_CH       ADC_CHANNEL_0
/* TIM3 CH1/CH2 — серво pan/tilt, PB4/PB5, AF1                             */
#define BOARD_SERVO_Port       GPIOB
#define BOARD_SERVO_PAN_Pin    GPIO_PIN_4
#define BOARD_SERVO_TILT_Pin   GPIO_PIN_5
#define BOARD_SERVO_AF         GPIO_AF1_TIM3
/* Кнопка MODE: PA8, EXTI8 (лінії 4..15 → EXTI4_15_IRQn), внутрішній pull-up */
#define BOARD_BTN_Port         GPIOA
#define BOARD_BTN_Pin          GPIO_PIN_8
#define BOARD_BTN_IRQn         EXTI4_15_IRQn
/* LD4 — користувацький LED, PA5 (UM2953 «LEDs»)                             */
#define BOARD_LED_Port         GPIOA
#define BOARD_LED_Pin          GPIO_PIN_5
/* MARK — службовий GPIO для тригера логічного аналізатора, PB3 (D3)         */
#define BOARD_MARK_Port        GPIOB
#define BOARD_MARK_Pin         GPIO_PIN_3

/* ---------- параметри шин (розрахунок — docs/pinout.md §4) ---------- */
#define BOARD_SYSCLK_HZ        48000000u       /* HSI48, HSIDIV=1, AHB=APB=1 */
/* I2C1 TIMINGR @ I2CCLK = PCLK = 48 МГц:
   PRESC=2 (62.5 нс), SCLDEL=3, SDADEL=0, SCLH=62, SCLL=93
   tLOW = 94·62.5 = 5.875 мкс, tHIGH = 63·62.5 = 3.94 мкс → ≈100 кГц (Sm) */
#define BOARD_I2C_TIMING_100K  0x20303E5Du
/* Варіант Fm 400 кГц (відхилено для ЛР2, див. pinout.md «Відхилені варіанти»):
   PRESC=1 (41.7 нс), SCLDEL=3, SDADEL=1, SCLH=17, SCLL=33 */
#define BOARD_I2C_TIMING_400K  0x10311121u
#define BOARD_I2C_TIMING       BOARD_I2C_TIMING_100K
/* write_read: 0 — STOP+START (перевірено записом ЛР1), 1 — repeated START */
#define BOARD_I2C_USE_RESTART  0
/* SPI1 = PCLK/128 = 375 кГц (ініціалізація SD ≤ 400 кГц), робоча — PCLK/8 = 6 МГц */
#define BOARD_SPI_PRESC_SLOW   SPI_BAUDRATEPRESCALER_128
#define BOARD_SPI_PRESC_FAST   SPI_BAUDRATEPRESCALER_8
/* TIM3: PSC=47 → 1 МГц (1 тік = 1 мкс), ARR=19999 → 50 Гц */
#define BOARD_TIM3_PSC         (48u - 1u)
#define BOARD_TIM3_ARR         (20000u - 1u)

/* ---------- хендли периферії (визначені в Core/Src/main.c) ---------- */
extern ADC_HandleTypeDef  hadc1;
extern I2C_HandleTypeDef  hi2c1;
extern SPI_HandleTypeDef  hspi1;
extern TIM_HandleTypeDef  htim3;
extern UART_HandleTypeDef huart2;

/* ---------- екземпляри шин для драйверів ---------- */
extern const bus_i2c_t board_i2c1;
extern const bus_spi_t board_sd_spi;

/* Запуск сервісів платформи після MX_*_Init (PWM, калібрування АЦП, RX UART) */
void board_start(void);
void board_uart_rx_irq(void);   /* з USART2_IRQHandler */
const char *board_hal_str(HAL_StatusTypeDef s);

#endif
