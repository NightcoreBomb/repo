/**
 * main.h — Lab 1 bring-up, NUCLEO-C031C6 (Wokwi)
 * Pin map (див. docs/hardware-components.md):
 *   USART2  PA2 TX / PA3 RX (AF1)   -> ST-Link VCP / Wokwi Serial Monitor
 *   I2C1    PB8 SCL / PB9 SDA (AF6) -> MPU-6050 (0x68) + SSD1306 OLED (0x3C)
 *   SPI1    PA1 SCK / PA6 MISO / PA7 MOSI (AF0), PB0 CS (GPIO) -> microSD
 *   ADC1    PA0 (IN0)               -> потенціометр
 *   TIM3    PB4 CH1 / PB5 CH2 (AF1) -> сервоприводи pan / tilt, 50 Гц
 *   GPIO    PA8 (EXTI, pull-up)     -> кнопка користувача
 *   GPIO    PA5                     -> LED LD4 (heartbeat)
 */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32c0xx_hal.h"

#define LED_Pin        GPIO_PIN_5
#define LED_GPIO_Port  GPIOA
#define BTN_Pin        GPIO_PIN_8
#define BTN_GPIO_Port  GPIOA
#define SD_CS_Pin      GPIO_PIN_0
#define SD_CS_GPIO_Port GPIOB

void Error_Handler(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
