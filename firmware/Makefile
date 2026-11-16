# Прошивка ЛР3 (super loop) — NUCLEO-C031C6 (Wokwi). Викликається з кореневого Makefile в Docker.
#
#  Шари (див. docs/architecture.md):
#    Core/        код у стилі CubeMX (init периферії, IRQ, main)
#    Drivers/     CMSIS + HAL від ST — НЕ редагуються, власні прапорці попереджень
#    Interfaces/  контракти: bus_i2c.h, bus_spi.h, plat.h, status.h
#    Platform/    адаптація до плати: board.h + реалізації шин над HAL
#    Devices/     драйвери пристроїв, залежать лише від Interfaces/
#    App/         прикладна логіка, не знає про HAL

TARGET    = firmware
BUILD_DIR = build
DEBUG    ?= 1
# -Os: з -Og прошивка ЛР2 не вміщується у 32 КБ Flash C031C6 (журнал проблем, П-3)
OPT      ?= -Os

# ---------- вендорський код ----------
HAL = Drivers/STM32C0xx_HAL_Driver/Src
# Лише модулі, увімкнені в stm32c0xx_hal_conf.h (обґрунтування — там само).
# ЛР3: I2C/SPI/ADC/TIM/UART — регістри напряму (журнал ЛР3, П-13)
VENDOR_SOURCES = \
  $(HAL)/stm32c0xx_hal.c \
  $(HAL)/stm32c0xx_hal_cortex.c \
  $(HAL)/stm32c0xx_hal_rcc.c \
  $(HAL)/stm32c0xx_hal_gpio.c \
  Core/Src/system_stm32c0xx.c \
  Core/Src/syscalls.c \
  Core/Src/sysmem.c

# ---------- власний код ----------
CORE_SOURCES     = Core/Src/main.c Core/Src/stm32c0xx_it.c Core/Src/stm32c0xx_hal_msp.c
PLATFORM_SOURCES = $(wildcard Platform/*.c)
# w25qxx.c — драйвер для Форми A; у Wokwi чипа немає, тож у прошивку не лінкується
# (тестується на хості, tests/test_w25qxx.c)
DEVICES_SOURCES  = Devices/mpu6050.c Devices/ssd1306.c Devices/font5x7.c \
                   Devices/sdcard.c Devices/servo.c Devices/pot.c Devices/button.c
APP_SOURCES      = $(wildcard App/*.c)
OWN_SOURCES      = $(CORE_SOURCES) $(PLATFORM_SOURCES) $(DEVICES_SOURCES) $(APP_SOURCES)

ASM_SOURCES = startup_stm32c031xx.s
LDSCRIPT    = STM32C031C6Tx_FLASH.ld

PREFIX = arm-none-eabi-
CC  = $(PREFIX)gcc
AS  = $(PREFIX)gcc -x assembler-with-cpp
CP  = $(PREFIX)objcopy
SZ  = $(PREFIX)size

# Cortex-M0+ не має FPU. (Для F411/F446: -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
MCU = -mcpu=cortex-m0plus -mthumb

C_DEFS = -DUSE_HAL_DRIVER -DSTM32C031xx
C_INCLUDES = \
  -ICore/Inc -IPlatform -IInterfaces -IDevices -IApp \
  -IDrivers/STM32C0xx_HAL_Driver/Inc \
  -IDrivers/STM32C0xx_HAL_Driver/Inc/Legacy \
  -IDrivers/CMSIS/Device/ST/STM32C0xx/Include \
  -IDrivers/CMSIS/Include

BASE_CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -std=gnu11 -fdata-sections -ffunction-sections
# ЛР3 §5.4: статична оцінка стека — build/*.su (кадр кожної функції)
BASE_CFLAGS += -fstack-usage
ifeq ($(DEBUG), 1)
BASE_CFLAGS += -g -gdwarf-2
endif
# Відтворювана збірка: прибираємо шляхи хоста з debug-інфо
BASE_CFLAGS += -ffile-prefix-map=$(CURDIR)=.

# Власний код: усі попередження, і вони — помилки (WERROR=0 щоб тимчасово вимкнути)
WERROR     ?= 1
OWN_WARN    = -Wall -Wextra -Wshadow -Wundef -Wstrict-prototypes -Wstack-usage=256
# Ідентифікатор збірки в банері й логах (ЛР3 §6: «та сама збірка»): хеш вихідних кодів
# власного шару — однаковий і в Docker, і поза ним; плюс git-хеш, якщо є
FW_SRC_HASH := $(shell cat $(sort $(wildcard Core/Src/*.c App/*.[ch] Devices/*.[ch] Platform/*.[ch] Interfaces/*.h)) 2>/dev/null | sha1sum | cut -c1-8)
FW_GIT      := $(shell git rev-parse --short HEAD 2>/dev/null || echo nogit)
C_DEFS     += -DFW_ID=\"$(FW_SRC_HASH)-$(FW_GIT)\"
# ЛР3 П-20: FW_ID «вшивається» лише в app.o — без цієї залежності зміна main.c не оновлювала
# банер (обидва прогони показали той самий fw=51601fab). Штамп змінюється разом з ідентифікатором.
FW_ID_STAMP := $(BUILD_DIR)/fw_id.stamp
$(shell mkdir -p $(BUILD_DIR); echo '$(FW_SRC_HASH)-$(FW_GIT)' | cmp -s - $(FW_ID_STAMP) 2>/dev/null || echo '$(FW_SRC_HASH)-$(FW_GIT)' > $(FW_ID_STAMP))
ifeq ($(WERROR), 1)
OWN_WARN   += -Werror
endif
# Вендорський код: -Wall -Wextra без -Werror (HAL 1.4.1 під ними чистий — перевірено).
# Додаткові -Wundef/-Wstrict-prototypes дають 49 попереджень: 48× -Wundef у
# stm32c0xx_hal_spi.c і 1× -Wstrict-prototypes у syscalls.c (журнал, П-2) —
# тому ці прапорці лише для власного коду.
# Якщо оновлення HAL принесе нові попередження, вони буде видно у виводі збірки.
VENDOR_WARN = -Wall -Wextra

ASFLAGS = $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections

LIBS    = -lc -lm -lnosys
# --no-warn-rwx-segments є лише в binutils >= 2.39 (Ubuntu 22.04 має 2.38)
LD_RWX := $(shell $(CC) -Wl,--help 2>/dev/null | grep -q no-warn-rwx-segments && echo -Wl,--no-warn-rwx-segments)
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -Wl,--print-memory-usage $(LD_RWX)

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OWN_OBJECTS    = $(addprefix $(BUILD_DIR)/,$(notdir $(OWN_SOURCES:.c=.o)))
VENDOR_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(VENDOR_SOURCES:.c=.o)))
ASM_OBJECTS    = $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
OBJECTS = $(OWN_OBJECTS) $(VENDOR_OBJECTS) $(ASM_OBJECTS)
vpath %.c $(sort $(dir $(OWN_SOURCES) $(VENDOR_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(OWN_OBJECTS):    CFLAGS = $(BASE_CFLAGS) $(OWN_WARN)
$(VENDOR_OBJECTS): CFLAGS = $(BASE_CFLAGS) $(VENDOR_WARN)

# V=1 — показати повні команди компілятора
V ?= 0
ifeq ($(V), 0)
Q = @
endif

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "  CC  $<"
	$(Q)$(CC) -c $(CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "  AS  $<"
	$(Q)$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "  LD  $@"
	$(Q)$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O binary -S $< $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
-include $(wildcard $(BUILD_DIR)/*.d)

# FW_ID (див. вище): app.o перекомпільовується, коли змінився ідентифікатор збірки
$(BUILD_DIR)/app.o: $(FW_ID_STAMP)
