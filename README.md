# ЛР1 — bring-up STM32 (Форма B, Wokwi)

NUCLEO-C031C6 · STM32 HAL · Docker-тулчейн · Wokwi for VS Code
Проєкт курсу: **Motion-Mirroring Teleoperator** (див. `docs/PRD.md`).

## Структура

```
docker/Dockerfile            тулчейн-образ (ubuntu 22.04 + arm-none-eabi-gcc)
firmware/                    прошивка: Core/ (main, msp, it), Drivers/ (HAL+CMSIS), Makefile, .ld, startup
diagram.json                 схема Wokwi: Nucleo + MPU-6050 + OLED + pot + кнопка + 2 серво + microSD + logic analyzer
wokwi.toml                   вказує Wokwi, який ELF запускати
Makefile                     обгортка: image / build / clean / sim / flash / monitor
docs/PRD.md                  PRD
docs/hardware-components.md  дослідження компонентів
docs/report.md               чернетка звіту
docs/screenshots/            скріншоти
.github/workflows/ci.yml     CI: make build на кожен push/PR
```

## Швидкий старт

Потрібні: Docker, make, VS Code + розширення **Wokwi for VS Code**.
Windows: працювати в **WSL2** (Ubuntu), Docker Desktop з WSL-інтеграцією, VS Code через «WSL: Connect».

```bash
make image      # один раз: зібрати тулчейн-образ
make build      # firmware/build/firmware.{elf,hex,bin}
```

Далі у VS Code: `F1 → Wokwi: Request a New License` (один раз), потім `F1 → Wokwi: Start Simulator`.

Очікуваний лог:

```
==== hello from STM32 ====
LR1 bring-up | NUCLEO-C031C6 | SYSCLK = 48000000 Hz
[init] USART2         HAL_OK
[init] I2C1           HAL_OK
...
[i2c ] scan 0x08..0x77: 0x3C 0x68  (2 device(s))
[i2c ] MPU-6050 WHO_AM_I = 0x68 (HAL_OK, expected 0x68)
[spi ] 80 probe clocks on SCK: HAL_OK
tick 1 | t=1xxx ms | adc=2047 (1649 mV) | btn=0
tick 2 | ...
```

Покрути потенціометр — змінюється `adc`; натисни кнопку — `[btn ] pressed`.

Headless (для CI): `export WOKWI_CLI_TOKEN=...; make sim` — чекає рядок `tick 3`.

## Форма A (залізо)

`make flash` (st-flash), `make monitor PORT=/dev/ttyACM0` (picocom). Для F411/F446 потрібен
CubeF4-пакет HAL і `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` — див. коментар у `firmware/Makefile`.

## Звіт у PDF

```bash
pandoc docs/report.md -o docs/report.pdf --pdf-engine=xelatex -V mainfont="DejaVu Serif"
```
або відкрити `report.md` у VS Code → Markdown Preview → друк у PDF.
