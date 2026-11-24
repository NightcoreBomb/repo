/**
 * board.h — ЄДИНЕ місце знання про плату NUCLEO-C031C6 + стенд ЛР2/ЛР3.
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
#define BOARD_POT_ADC_CH       0u              /* CHSELR біт 0 */
/* TIM3 CH1/CH2 — серво pan/tilt, PB4/PB5, AF1                             */
#define BOARD_SERVO_Port       GPIOB
#define BOARD_SERVO_PAN_Pin    GPIO_PIN_4
#define BOARD_SERVO_TILT_Pin   GPIO_PIN_5
#define BOARD_SERVO_AF         GPIO_AF1_TIM3
/* Кнопка MODE: PA8, EXTI8 обидва фронти (лічильник брязкоту), debounce — опитування 1 кГц; pull-up */
#define BOARD_BTN_Port         GPIOA
#define BOARD_BTN_Pin          GPIO_PIN_8
#define BOARD_BTN_IRQn         EXTI4_15_IRQn
/* LD4 — користувацький LED, PA5 (UM2953 «LEDs»)                             */
#define BOARD_LED_Port         GPIOA
#define BOARD_LED_Pin          GPIO_PIN_5
/* MARK — службовий GPIO для тригера логічного аналізатора, PB3 (D3)         */
#define BOARD_MARK_Port        GPIOB
#define BOARD_MARK_Pin         GPIO_PIN_3
/* ЛР3: службові GPIO для вимірювань (методичка ЛР3 §5.3), вільні піни Arduino:
   PROBE0 PA9 (D8)  — «IMU»: 1 на час imu_task (ширина = час, фронти = період/джитер)
   PROBE1 PA10 (D2) — «PWM»: імпульс у момент запису нового CCR серво (реакція)
   PROBE2 PA15 (D7) — «LONG»: 1 на час довгої операції (SD-блок, OLED, dump, calib) */
#define BOARD_PROBE_Port       GPIOA
#define BOARD_PROBE0_Pin       GPIO_PIN_9
#define BOARD_PROBE1_Pin       GPIO_PIN_10
#define BOARD_PROBE2_Pin       GPIO_PIN_15

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
/* write_read: STOP+START (перевірено записом ЛР1; MPU-6050 зберігає вказівник) */
/* Після обміну байтом чекати скидання TXIS/RXNE (Wokwi, П-11), мкс */
#define BOARD_I2C_GUARD_US     200u
/* SPI1 = PCLK/128 = 375 кГц (ініціалізація SD ≤ 400 кГц), робоча — PCLK/8 = 6 МГц.
   Значення — поле BR[2:0] регістра CR1 (RM0490: 110 = /128, 010 = /8) */
#define BOARD_SPI_BR_SLOW      (6u << SPI_CR1_BR_Pos)
#define BOARD_SPI_BR_FAST      (2u << SPI_CR1_BR_Pos)
/* I²C (ЛР3): після START з ≥2 байтами — пауза до кінця фази адреси, перш ніж
   класти перший байт у TXDR (П-11). Адреса @100 кГц = 9 біт × 10 мкс = 90 мкс.
   У ЛР2 тут було «≥ 1 мс за HAL_GetTick» (лише мс-таймер); тепер є лічильник тактів. */
#define BOARD_I2C_ADDR_GUARD_US 300u
/* USART2 BRR = 48e6 / 115200 = 416.67 → 417 (−0.08 %) */
#define BOARD_UART_BRR         417u
/* TIM3: PSC=47 → 1 МГц (1 тік = 1 мкс), ARR=19999 → 50 Гц */
#define BOARD_TIM3_PSC         (48u - 1u)
#define BOARD_TIM3_ARR         (20000u - 1u)

/* ---------- периферія (ЛР3): регістри напряму, без HAL-хендлів ----------
   HAL_I2C/SPI/ADC/TIM/UART займали ~9 КБ із 32 КБ Flash (журнал ЛР3, П-13);
   ініціалізація — Core/Src/main.c (MX_*_Init у стилі LL). */
#define BOARD_I2C              I2C1
#define BOARD_SPI              SPI1
#define BOARD_UART             USART2
#define BOARD_PWM_TIM          TIM3

/* ---------- екземпляри шин для драйверів ---------- */
extern const bus_i2c_t board_i2c1;
extern const bus_spi_t board_sd_spi;

/* Обробники, що викликаються з Core/Src/stm32c0xx_it.c */
void board_uart_rx_irq(void);   /* USART2_IRQHandler */
void board_btn_exti(void);      /* EXTI4_15_IRQHandler */
int  board_adc_calibrate(void); /* ADCAL; 0 — OK */

/* ЛР4: службове для Platform/ і Core/ (не для App) */
void plat_log_raw(const char *fmt, ...) __attribute__((format(printf, 1, 2)));  /* без м'ютекса */
void os_uart_lock(void);
void os_uart_unlock(void);
void os_tick_account(uint32_t systick_val_on_entry);   /* вартість тіку (SysTick_Handler) */

#endif
