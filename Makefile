# Кореневий Makefile — обгортка над Docker-тулчейном
# Форма B (Wokwi): make image -> make build -> F1 "Wokwi: Start Simulator" у VS Code
#                  або headless: make sim (потрібен wokwi-cli + WOKWI_CLI_TOKEN)

IMAGE  ?= stm32-build
ELF     = firmware/build/firmware.elf
BIN     = firmware/build/firmware.bin
PORT   ?= /dev/ttyACM0
# $$ — щоб id -u виконався в shell, а не розкрився make'ом як порожня змінна
DOCKER  = docker run --rm -v "$(CURDIR)":/workspace -w /workspace \
          --user $$(id -u):$$(id -g) $(IMAGE)

.PHONY: image build clean flash monitor sim shell

image:                      ## зібрати тулчейн-образ (один раз)
	docker build -t $(IMAGE) docker/

build:                      ## скомпілювати прошивку в контейнері
	$(DOCKER) make -C firmware all -j4

clean:
	$(DOCKER) make -C firmware clean

shell:                      ## інтерактивний shell у тулчейн-контейнері
	docker run --rm -it -v "$(CURDIR)":/workspace -w /workspace --user $$(id -u):$$(id -g) $(IMAGE) bash

flash: build                ## Форма A: залити на Nucleo через ST-Link
	st-flash --reset write $(BIN) 0x08000000

monitor:                    ## Форма A: UART-термінал (вихід: Ctrl-A Ctrl-X)
	picocom -b 115200 --imap lfcrlf $(PORT)

sim: build                  ## Форма B: headless-прогін у Wokwi, чекає "tick 3"
	wokwi-cli --timeout 8000 --expect-text "tick 3" .
