# Кореневий Makefile — усе через Docker-тулчейн (однаковий результат на macOS/Linux/WSL2/CI).
#
#   make image      зібрати образ тулчейну (один раз)
#   make build      перевірки шарів і pinout + прошивка (-Wall -Wextra -Werror для власного коду)
#   make test       хостові тести (Unity) + тести інструментів (python)
#   make coverage   покриття Devices/ + App/ з порогом (tests/build/coverage/index.html)
#   make check      лише перевірки: check_deps + check_pinout
#   make sim        headless Wokwi: повний self-test (WOKWI_CLI_TOKEN)
#   make sim-imu    headless Wokwi: FR-02 — кути в статиці (set-control + status)
#   make lab3       headless Wokwi: усі сценарії ЛР3 (tools/run_lab3.sh), WOKWI_CLI_TOKEN
#   make probe P=bus|pwm [GATE=1]   переставити щупи аналізатора в diagram.json
#   make capture NAME=i2c_mpu6050_burst   wokwi.vcd → docs/captures/NAME.vcd + декодування
#   make decode VCD=docs/captures/x.vcd   текстове декодування запису (sigrok-cli + vcdtool)
#   make vectors    перегенерувати tests/vectors/capture_vectors.h з docs/captures/*.vcd
#   make flash / monitor   Форма A

IMAGE  ?= stm32-build
ELF     = firmware/build/firmware.elf
BIN     = firmware/build/firmware.bin
PORT   ?= /dev/ttyACM0
# $$ — щоб id -u виконався в shell, а не розкрився make'ом як порожня змінна
DOCKER  = docker run --rm -v "$(CURDIR)":/workspace -w /workspace \
          --user $$(id -u):$$(id -g) -e HOME=/tmp $(IMAGE)
# NODOCKER=1 — запуск тими самими командами без контейнера (CI вже всередині образу)
ifeq ($(NODOCKER),1)
DOCKER  =
endif

.PHONY: image build fw check test coverage sim sim-imu lab3 lint probe capture decode vectors \
        clean shell flash monitor versions

image:
	docker build -t $(IMAGE) docker/

check:
	$(DOCKER) python3 tools/check_deps.py firmware
	$(DOCKER) python3 tools/check_pinout.py

build: check
	$(DOCKER) make -C firmware all -j4

test:
	$(DOCKER) make -C tests run
	$(DOCKER) python3 -m unittest -q tools/test_tools.py

coverage:
	$(DOCKER) make -C tests coverage

versions:
	$(DOCKER) sh -c 'arm-none-eabi-gcc --version | head -1; gcc --version | head -1; \
	  python3 --version; lcov --version | head -1; sigrok-cli --version | head -1'

clean:
	$(DOCKER) make -C firmware clean
	$(DOCKER) make -C tests clean

shell:
	docker run --rm -it -v "$(CURDIR)":/workspace -w /workspace --user $$(id -u):$$(id -g) $(IMAGE) bash

# ---------- Wokwi (Форма B) ----------
SIM_TIMEOUT ?= 30000
sim: build
	mkdir -p build
	wokwi-cli --timeout $(SIM_TIMEOUT) --scenario tests/sim/selftest.test.yaml \
	          --serial-log-file build/serial.log .
	python3 tools/parse_selftest.py build/serial.log

sim-imu: build
	mkdir -p build
	wokwi-cli --timeout $(SIM_TIMEOUT) --scenario tests/sim/lr3_fr02_static.yaml \
	          --serial-log-file build/serial-imu.log .

lab3: build
	tools/run_lab3.sh

lint:
	wokwi-cli lint .

probe:
	python3 tools/probe.py $(or $(P),bus) $(if $(GATE),--gate,)

# Після зупинки симуляції VS Code пише wokwi.vcd у корінь проєкту
capture:
	@test -n "$(NAME)" || (echo "usage: make capture NAME=<інтерфейс>_<пристрій>_<операція>"; exit 1)
	mv wokwi.vcd docs/captures/$(NAME).vcd
	python3 tools/decode_capture.py docs/captures/$(NAME).vcd > docs/captures/$(NAME).txt
	@echo "saved docs/captures/$(NAME).vcd + .txt"

decode:
	$(DOCKER) python3 tools/decode_capture.py $(VCD)

vectors:
	$(DOCKER) sh -c 'python3 tools/vcd2vectors.py docs/captures/*.vcd > tests/vectors/capture_vectors.h'

# ---------- Форма A ----------
flash: build
	st-flash --reset write $(BIN) 0x08000000

monitor:
	picocom -b 115200 --imap lfcrlf $(PORT)
