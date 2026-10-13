
TARGET    = firmware
BUILD_DIR = build
DEBUG    ?= 1
OPT      ?= -Og

HAL = Drivers/STM32C0xx_HAL_Driver/Src
C_SOURCES = \
  Core/Src/main.c \
  Core/Src/stm32c0xx_it.c \
  Core/Src/stm32c0xx_hal_msp.c \
  Core/Src/system_stm32c0xx.c \
  Core/Src/syscalls.c \
  Core/Src/sysmem.c \
  $(HAL)/stm32c0xx_hal.c \
  $(HAL)/stm32c0xx_hal_cortex.c \
  $(HAL)/stm32c0xx_hal_rcc.c \
  $(HAL)/stm32c0xx_hal_rcc_ex.c \
  $(HAL)/stm32c0xx_hal_flash.c \
  $(HAL)/stm32c0xx_hal_flash_ex.c \
  $(HAL)/stm32c0xx_hal_gpio.c \
  $(HAL)/stm32c0xx_hal_exti.c \
  $(HAL)/stm32c0xx_hal_dma.c \
  $(HAL)/stm32c0xx_hal_dma_ex.c \
  $(HAL)/stm32c0xx_hal_pwr.c \
  $(HAL)/stm32c0xx_hal_pwr_ex.c \
  $(HAL)/stm32c0xx_hal_uart.c \
  $(HAL)/stm32c0xx_hal_uart_ex.c \
  $(HAL)/stm32c0xx_hal_i2c.c \
  $(HAL)/stm32c0xx_hal_i2c_ex.c \
  $(HAL)/stm32c0xx_hal_spi.c \
  $(HAL)/stm32c0xx_hal_spi_ex.c \
  $(HAL)/stm32c0xx_hal_adc.c \
  $(HAL)/stm32c0xx_hal_adc_ex.c \
  $(HAL)/stm32c0xx_hal_tim.c \
  $(HAL)/stm32c0xx_hal_tim_ex.c

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
  -ICore/Inc \
  -IDrivers/STM32C0xx_HAL_Driver/Inc \
  -IDrivers/STM32C0xx_HAL_Driver/Inc/Legacy \
  -IDrivers/CMSIS/Device/ST/STM32C0xx/Include \
  -IDrivers/CMSIS/Include

ASFLAGS = $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS  = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Відтворювана збірка: прибираємо шляхи хоста з debug-інфо
CFLAGS += -ffile-prefix-map=$(CURDIR)=.
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

LIBS    = -lc -lm -lnosys
# --no-warn-rwx-segments є лише в binutils >= 2.39 (Ubuntu 22.04 має 2.38) — додаємо, тільки якщо ld його знає
LD_RWX := $(shell $(CC) -Wl,--help 2>/dev/null | grep -q no-warn-rwx-segments && echo -Wl,--no-warn-rwx-segments)
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections $(LD_RWX)

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
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
