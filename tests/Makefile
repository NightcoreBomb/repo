# Хостові тести (gcc, без плати). Запуск: make -C tests  /  з кореня: make test
FW      = ../firmware
CC     ?= gcc
BUILD   = build
UNITY   = vendor/unity
CFLAGS  = -std=gnu11 -O0 -g -Wall -Wextra -Werror -Wshadow -Wundef \
          -I$(FW)/Interfaces -I$(FW)/Devices -I$(FW)/App -I. -Idoubles -I$(UNITY) \
          -DUNITY_INCLUDE_DOUBLE -DUNITY_OUTPUT_COLOR
# Покриття вимірюється лише для коду, що тестується (Devices/, App/)
COVFLAGS = --coverage -fprofile-update=atomic

DEV  = $(FW)/Devices
APP  = $(FW)/App
DBL  = doubles

# test_<name>: перелік файлів під тестом + потрібні дублери
SRC_mpu6050  = $(DEV)/mpu6050.c $(DBL)/mock_i2c.c $(DBL)/fake_mpu6050.c $(DBL)/stub_i2c.c
SRC_ssd1306  = $(DEV)/ssd1306.c $(DEV)/font5x7.c $(DBL)/mock_i2c.c $(DBL)/stub_i2c.c
SRC_sdcard   = $(DEV)/sdcard.c $(DBL)/spi_log.c $(DBL)/fake_sd.c
SRC_w25qxx   = $(DEV)/w25qxx.c $(DBL)/spi_log.c $(DBL)/fake_w25q.c
SRC_conv     = $(DEV)/servo.c $(DEV)/pot.c
SRC_app      = $(APP)/app.c $(APP)/selftest.c $(APP)/capture.c \
               $(DEV)/mpu6050.c $(DEV)/ssd1306.c $(DEV)/font5x7.c $(DEV)/sdcard.c \
               $(DEV)/servo.c $(DEV)/pot.c \
               $(DBL)/fake_mpu6050.c $(DBL)/spi_log.c $(DBL)/fake_sd.c $(DBL)/fake_plat.c

TESTS = mpu6050 ssd1306 sdcard w25qxx conv app
BINS  = $(addprefix $(BUILD)/test_,$(TESTS))

all: run

$(BUILD):
	mkdir -p $@

.SECONDEXPANSION:
$(BUILD)/test_%: test_%.c $$(SRC_$$*) $(UNITY)/unity.c $(wildcard $(FW)/*/*.h doubles/*.h) | $(BUILD)
	@rm -f $(BUILD)/test_$*-*.gcda
	$(CC) $(CFLAGS) $(COVFLAGS) -o $@ $< $(SRC_$*) $(UNITY)/unity.c

# vectors з реальних записів — перегенеровуються, якщо змінились .vcd
vectors/capture_vectors.h: $(wildcard ../docs/captures/*.vcd) ../tools/vcd2vectors.py ../tools/vcdtool.py
	python3 ../tools/vcd2vectors.py $(wildcard ../docs/captures/*.vcd) > $@

$(BUILD)/test_mpu6050: vectors/capture_vectors.h

run: $(BINS)
	@fail=0; for t in $(BINS); do echo "== $$t"; ./$$t || fail=1; done; \
	if [ $$fail -ne 0 ]; then echo "TESTS FAILED"; exit 1; fi; echo "ALL TESTS PASSED"

# Звіт покриття: build/coverage/index.html + підсумок у консолі. Поріг — COV_MIN.
COV_MIN ?= 85
coverage: clean run
	lcov --quiet --capture --directory $(BUILD) --output-file $(BUILD)/all.info \
	     --rc branch_coverage=1 --ignore-errors inconsistent,unused 2>/dev/null || \
	lcov --quiet --capture --directory $(BUILD) --output-file $(BUILD)/all.info --rc lcov_branch_coverage=1
	lcov --quiet --extract $(BUILD)/all.info '*/firmware/Devices/*' '*/firmware/App/*' \
	     --output-file $(BUILD)/cov.info --rc branch_coverage=1 --ignore-errors unused 2>/dev/null || \
	lcov --quiet --extract $(BUILD)/all.info '*/firmware/Devices/*' '*/firmware/App/*' \
	     --output-file $(BUILD)/cov.info --rc lcov_branch_coverage=1
	lcov --list $(BUILD)/cov.info --rc branch_coverage=1 2>/dev/null || lcov --list $(BUILD)/cov.info
	genhtml --quiet $(BUILD)/cov.info --output-directory $(BUILD)/coverage >/dev/null 2>&1 || true
	@python3 ../tools/cov_gate.py $(BUILD)/cov.info $(COV_MIN)

clean:
	rm -rf $(BUILD)

.PHONY: all run coverage clean
