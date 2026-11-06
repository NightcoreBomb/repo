# Motion-Mirroring Teleoperator — STM32 (Форма B, Wokwi)

NUCLEO-C031C6 · STM32 HAL · Docker-тулчейн · Wokwi for VS Code · курс «Системне програмування вбудованих систем»

| ЛР | Що | Документи |
|---|---|---|
| 1 | bring-up: build → flash → monitor, PRD, компоненти | `docs/PRD.md`, `docs/hardware-components.md` |
| 2 | інтеграція сенсорів, pinout, логічний аналізатор, архітектура, тести | `docs/lab2-report.md` ← почати звідси |

## Структура

```
docker/Dockerfile          тулчейн: arm-none-eabi-gcc, gcc+lcov (тести), python3, sigrok-cli
firmware/                  прошивка, шари (docs/architecture.md):
  Core/                      CubeMX-стиль: main, MSP, IRQ
  Drivers/                   HAL 1.4.1 + CMSIS (vendored, docs/third_party.md)
  Interfaces/                контракти: bus_i2c.h, bus_spi.h, plat.h, status.h
  Platform/                  board.h (єдине місце пінів) + реалізації шин над HAL
  Devices/                   mpu6050, ssd1306, sdcard, w25qxx, servo, pot
  App/                       self-test, консоль, сценарії запису для аналізатора
tests/                     хостові тести (Unity), дублери шин, вектори з записів, сценарії Wokwi (tests/sim)
tools/                     check_deps, check_pinout, probe, vcdtool, decode_capture, vcd2vectors, parse_selftest
docs/pinout.md             pinout, адреси, живлення, розрахунок шин, конфлікти, інструкція підключення
docs/architecture.md       шари, правила залежностей, component diagram, test doubles
docs/captures/             записи аналізатора (.vcd + .txt), процедура, аналіз
diagram.json, wokwi.toml   стенд Wokwi
.github/workflows/ci.yml   check + build + test + coverage на кожен push/PR; Wokwi — якщо є WOKWI_CLI_TOKEN
```

## Швидкий старт

Потрібні Docker, make, VS Code + **Wokwi for VS Code** (Windows — у WSL2).

```bash
make image        # один раз
make build        # перевірки шарів і pinout + прошивка → firmware/build/firmware.{elf,hex,bin}
make test         # хостові тести, плата не потрібна
make coverage     # покриття Devices/ + App/ з порогом → tests/build/coverage/index.html
```

VS Code: `F1 → Wokwi: Start Simulator`. Очікуваний початок лога:

```
==== hello from STM32 ====
LR2 sensor integration | NUCLEO-C031C6 | SYSCLK = 48000000 Hz
[init] USART2         HAL_OK
...
ST BEGIN fw=LR2 board=NUCLEO-C031C6 sysclk=48000000
ST imu PASS id=0x68 pwr=0x01 ax=0 ay=0 az=1000 an=1000 gx=0 gy=0 gz=0 t_cC=2400 c_norm=1 c_gyro=1 c_temp=1 c_live=1
ST oled PASS addr=0x3C
ST sd PASS type=SDHC ...
ST servo PASS pan_us=1500 tilt_us=1500 period_us=20000
ST pot PASS raw=... mv=... dz_cdeg=...
ST btn PASS level=1 presses=0
ST END pass=6 fail=0
==== bring-up done, entering main loop (type 'help') ====
tick 1 | t=... ms | az=1000 mg | adc=... | btn=0
```

Команди в Serial Monitor: `help`, `selftest`, `stimu`, `imu`, `imuchk ax ay az [gx gy gz [t_cC]]`,
`scan`, `servo <pan_us> <tilt_us>`, `status`, `cap i2c|nack|rs|spi|spiw|pwm|adc|uart|all`.

Headless (потрібен `WOKWI_CLI_TOKEN`):

```bash
make sim          # повний self-test + розбір лога (tools/parse_selftest.py)
make sim-imu      # сценарій MPU-6050: set-control + imuchk (tests/sim/mpu6050.test.yaml)
```

## Записи логічного аналізатора

```bash
make probe P=bus            # або pwm; GATE=1 — запис лише поки MARK=1
# симуляція → команда cap ... → стоп симуляції → wokwi.vcd
make capture NAME=i2c_mpu6050_burst
make vectors && make test   # байти з запису стають тестовими векторами
```

Деталі — `docs/captures/README.md`.

## Якщо щось не так

* **Wokwi лає `nucleo:PB3`** (невідомий пін): замініть у `tools/probe.py` `PB3` на `D3` і `make probe`.
* **Скан I²C довгий (~0.7 с)** — особливість HAL+Wokwi після NACK, див. `docs/captures/analysis.md`, А-2.
* **`make build` падає на `check_pinout`** — таблиця `docs/pinout.md`, `diagram.json` і `board.h`
  розійшлися; повідомлення каже, де саме.

## Форма A (залізо)

`make flash` (st-flash), `make monitor PORT=/dev/ttyACM0`. Для F411/F446 потрібен пакет HAL STM32CubeF4,
startup/linker і `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` (коментар у `firmware/Makefile`);
Interfaces/Devices/App переносяться без змін — міняються лише `Platform/` і `Core/`.

## Звіт у PDF

```bash
pandoc docs/lab2-report.md -o docs/lab2-report.pdf --pdf-engine=xelatex -V mainfont="DejaVu Serif"
```
