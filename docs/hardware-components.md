# Дослідження компонентів (Крок 5)

Форма виконання: **B (Wokwi)**. Цільова плата: **NUCLEO-C031C6** — єдина Nucleo-64, яку симулює Wokwi
(F411RE/F446RE у Wokwi відсутні, див. `report.md`, Проблема 1). Код написаний на STM32 HAL, тож перенос
на F411/F446 зводиться до заміни HAL-пакета, startup/linker-файлів і прапорців `-mcpu`.

## 1. Мікроконтролер — STM32C031C6T6 (на NUCLEO-C031C6)

| Параметр | Значення | Джерело |
|---|---|---|
| Ядро | Arm Cortex-M0+, 48 МГц, **без FPU** | DS13867, розд. 1 «Description» |
| Пам'ять | 32 КБ Flash, 12 КБ SRAM | DS13867, розд. 1 |
| Живлення | 2.0–3.6 В (на Nucleo — 3.3 В від LDO) | DS13867, «General operating conditions» |
| Тактування | HSI48 (внутрішній RC 48 МГц), HSIDIV=1 → SYSCLK 48 МГц | RM0490, розд. «Reset and clock control (RCC)» |
| VCP | USART2: PA2 TX / PA3 RX → ST-Link | UM2953, розд. «USART virtual communication» |
| LED LD4 | PA5 (активний HIGH) | UM2953, розд. «LEDs» |

### Розподіл пінів (alternate functions)

| Інтерфейс | Пін | AF | Призначення | Джерело |
|---|---|---|---|---|
| USART2_TX / RX | PA2 / PA3 | AF1 | лог, 115200 8N1 | DS13867, табл. «Port A alternate function mapping» |
| I2C1_SCL / SDA | PB8 / PB9 | AF6 | MPU-6050 + OLED | DS13867, табл. «Port B alternate function mapping» |
| SPI1_SCK | PA1 | AF0 | microSD (SCK не на PA5, бо там LED) | DS13867, Port A AF |
| SPI1_MISO / MOSI | PA6 / PA7 | AF0 | microSD | DS13867, Port A AF |
| SD_CS | PB0 | GPIO out | програмний NSS | — |
| ADC1_IN0 | PA0 | analog | потенціометр | DS13867, табл. «Pin definitions» |
| TIM3_CH1 / CH2 | PB4 / PB5 | AF1 | PWM серво pan / tilt | DS13867, Port B AF |
| EXTI8 | PA8 | GPIO in + EXTI | кнопка MODE | RM0490, розд. «EXTI» |

## 2. Сенсори та виконавчі пристрої

| Компонент | Інтерфейс / параметри | Розводка, живлення | Регістри / команди для ідентифікації і старту | Споживання | Джерело |
|---|---|---|---|---|---|
| **MPU-6050** (GY-521) — 6-DOF IMU | I²C, адреса **0x68** (AD0=0) / 0x69 (AD0=1); до 400 кГц (у нас 100 кГц) | VDD 2.375–3.46 В; модуль GY-521 має LDO, тож VCC 3.3–5 В. SCL→PB8, SDA→PB9. На GY-521 вже є підтяжки 4.7 кОм | `WHO_AM_I` 0x75 → **0x68** (RM стор. 45); `PWR_MGMT_1` 0x6B = 0x00 — вивести зі sleep (RM стор. 40); дані `ACCEL_XOUT_H` 0x3B…0x48 (RM стор. 29–31) | ≈3.9 мА (accel+gyro) | PS-MPU-6000A rev 3.4, розд. 6 «Electrical Characteristics», розд. 9 «I²C interface»; RM-MPU-6000A rev 4.2 |
| **SSD1306** OLED 128×64 | I²C, адреса **0x3C** (SA0=0) / 0x3D; до 400 кГц | 3.3 В (модуль з DC-DC). На тій самій шині I²C1 | Перший байт = control byte (0x00 команда / 0x40 дані); `0xAE` display off, `0x8D 0x14` charge pump, `0xAF` display on | ~10–20 мА (залежить від к-сті засвічених пікселів) | SSD1306 datasheet rev 1.1, розд. 8.1.5 «I²C-bus interface», розд. 10 «Command descriptions» |
| **microSD** (SPI-режим) | SPI mode 0 (CPOL=0, CPHA=0), init-частота **100–400 кГц** (у нас 375 кГц), після init до 25 МГц | **Тільки 3.3 В!** DO→PA6, DI→PA7, SCK→PA1, CS→PB0 | ≥74 тактів з CS=high, потім `CMD0` (GO_IDLE_STATE) → R1=0x01, `CMD8` → перевірка версії, `ACMD41` до R1=0x00 | до ~100 мА піково при записі — на залізі від 3V3 Nucleo можна, але бажано конденсатор 10 мкФ біля карти | SD Physical Layer Simplified Spec, розд. 7 «SPI Mode», розд. 6.4.1 «Power Up» |
| **Потенціометр** 10 кОм (лінійний) | ADC, 12 біт, діапазон 0…VDDA (3.3 В) → 0…4095 | VCC→3V3, GND, SIG→PA0. Не подавати 5 В на PA0! | Ідентифікації немає; читання `HAL_ADC_Start/PollForConversion/GetValue` | 0.33 мА (3.3 В / 10 кОм) | RM0490, розд. «ADC»; DS13867 — характеристики ADC |
| **Кнопка** (MODE) | GPIO input, EXTI по спаду, внутрішній pull-up | один вивід → PA8, другий → GND | — | ~0.08 мА при натисканні (через pull-up ~40 кОм) | RM0490, розд. «GPIO», «EXTI» |
| **Сервопривід SG90/SG92R** ×2 | PWM 50 Гц (період 20 мс), імпульс ~0.5–2.4 мс, центр 1.5 мс | V+ 4.8–6 В — **від 5V, не від 3V3**; PWM→PB4/PB5 (3.3 В логіки достатньо) | — | до ~250 мА у русі, пікові сотні мА — на залізі потрібне окреме 5 В джерело зі спільною GND | Tower Pro SG90 datasheet |

## 3. Чому саме такий набір

- **MPU-6050** — головний сенсор проєкту (орієнтація кисті). Є і в наборі для Форми A, і у Wokwi — код переноситься без змін.
- **Потенціометр (ADC)** — ручка «чутливість/dead-zone» без перекомпіляції; в Wokwi емулює будь-який аналоговий датчик.
- **Кнопка (GPIO/EXTI)** — перемикання режимів Live → Record → Playback.
- **microSD (SPI)** — у Wokwi немає W25Q64, тож SPI-сховище траєкторії реалізуємо на microSD. Протокольно це той самий SPI master; на залізі замінюється на W25Q64 (JEDEC ID `0x9F` → `EF 40 17`).
- **SSD1306 (I²C)** — показує режим і кути; свідомо на тій самій шині, що й MPU, — це тренує роботу кількох пристроїв на одній I²C-шині.

## 4. Перевірка з логічним аналізатором (Крок 6)

Канали `wokwi-logic-analyzer` у `diagram.json`:

| D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7 |
|---|---|---|---|---|---|---|---|
| SCL (PB8) | SDA (PB9) | SCK (PA1) | MOSI (PA7) | CS (PB0) | PWM pan (PB4) | BTN (PA8) | UART TX (PA2) |

Під час `i2c_scan()` на D0 має бути видно пачки по 9 тактів SCL на кожну адресу; на D2 — 80 тактів SCK
при низькому CS (D4); на D5 — імпульс 1.5 мс кожні 20 мс. Файл `.vcd` відкривається у PulseView.

## Джерела

1. STMicroelectronics, *DS13867 — STM32C031x4/x6 datasheet*. https://www.st.com/en/microcontrollers-microprocessors/stm32c031c6.html
2. STMicroelectronics, *RM0490 — STM32C0x1 reference manual*. (там само, вкладка Documentation)
3. STMicroelectronics, *UM2953 — STM32C0 Nucleo-64 board user manual*. https://www.st.com/en/evaluation-tools/nucleo-c031c6.html
4. InvenSense, *MPU-6000/MPU-6050 Product Specification, PS-MPU-6000A-00 rev 3.4*.
5. InvenSense, *MPU-6000/MPU-6050 Register Map and Descriptions, RM-MPU-6000A-00 rev 4.2*.
6. Solomon Systech, *SSD1306 Advance Information, rev 1.1*.
7. SD Association, *SD Specifications Part 1: Physical Layer Simplified Specification*. https://www.sdcard.org/downloads/pls/
8. Wokwi Docs: board-st-nucleo-c031c6, wokwi-mpu6050, wokwi-microsd-card, wokwi-logic-analyzer. https://docs.wokwi.com
