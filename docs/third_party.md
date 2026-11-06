# Сторонні компоненти і версії

Мета: `make build` / `make test` через рік мають дати той самий результат.

| Компонент | Версія | Як зафіксовано | Де |
|---|---|---|---|
| STM32C0 HAL Driver | **1.4.1** (`__STM32C0xx_HAL_VERSION` 0x01040100) | vendoring (копія в репозиторії, не редагується) | `firmware/Drivers/STM32C0xx_HAL_Driver/` |
| CMSIS Device STM32C0 | **1.4.1** (`__STM32C0_CMSIS_VERSION`) | vendoring | `firmware/Drivers/CMSIS/Device/ST/STM32C0xx/` |
| CMSIS-Core(M) | **6.1** (`__CM_CMSIS_VERSION`) | vendoring | `firmware/Drivers/CMSIS/Include/` |
| Startup / linker script | з пакета STM32CubeC0 для STM32C031C6 | vendoring | `firmware/startup_stm32c031xx.s`, `STM32C031C6Tx_FLASH.ld` |
| Unity | **v2.6.0** | vendoring + sha256 архіву | `tests/vendor/unity/VERSION` |
| Тулчейн | Ubuntu 22.04: gcc-arm-none-eabi 10.3-2021.10, gcc 11, lcov 1.15, python 3.10, sigrok-cli 0.7.2 | Docker-образ `docker/Dockerfile`; фактичні версії друкує `make versions` у CI | `docker/Dockerfile` |
| Wokwi part types | `board-st-nucleo-c031c6`, `wokwi-mpu6050`, `board-ssd1306`, `wokwi-microsd-card`, `wokwi-servo`, `wokwi-potentiometer`, `wokwi-pushbutton`, `wokwi-logic-analyzer` | версія симулятора не фіксується (хмарний сервіс) — поведінку, на яку ми покладаємось, закріплено сценаріями `tests/sim/*.yaml` | `diagram.json` |

Перевірено при розробці: прошивка збирається без попереджень і з gcc-arm-none-eabi 10.3.1
(як у Docker; Flash 27 336 Б / 32 КБ), і з 13.2.1 (29 164 Б). Різниця розміру — оптимізатор,
поведінка однакова. Запас Flash ≈ 10–15 % — див. журнал проблем, П-3.
