ifeq ($(OS),Windows_NT)
SHELL := bash
endif

CROSS ?= arm-none-eabi-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE := $(CROSS)size
FLASH_TOOL ?= STM32_Programmer_CLI

TARGET := Output/STM32_CanhBao
ARCH := -mcpu=cortex-m3 -mthumb
RTOS := Lib/freertos
PORT := $(RTOS)/portable/GCC/ARM_CM3
DEVICE := Lib/stm32f10x_lib
CMSIS := Lib/cmsis/CMSIS/Core/Include
LINKER := Linker/stm32f103c8.ld

CPPFLAGS += -DSTM32F103xB -IUser/inc -IMyLib -I$(RTOS)/include \
            -I$(PORT) -I$(DEVICE)/Include -I$(CMSIS)
CFLAGS += $(ARCH) -std=c11 -Os -g3 -Wall -Wextra -ffunction-sections -fdata-sections
LDFLAGS += $(ARCH) -nostartfiles --specs=nano.specs --specs=nosys.specs \
           -T$(LINKER) -Wl,--gc-sections,-Map=$(TARGET).map

C_SRC := $(wildcard User/src/*.c MyLib/*.c Startup/*.c) \
         $(DEVICE)/Source/Templates/system_stm32f1xx.c \
         $(RTOS)/tasks.c $(RTOS)/queue.c $(RTOS)/list.c \
         $(PORT)/port.c $(RTOS)/portable/MemMang/heap_4.c
ASM_SRC := Startup/startup_stm32f103xb.s
OBJECTS := $(patsubst %.c,Output/%.o,$(C_SRC)) $(patsubst %.s,Output/%.o,$(ASM_SRC))

.PHONY: all clean flash
all: $(TARGET).elf $(TARGET).hex $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJECTS) $(LINKER)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

Output/%.o: %.c
	@mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

Output/%.o: %.s
	@mkdir -p "$(@D)"
	$(CC) $(ARCH) -c $< -o $@

flash: $(TARGET).hex
	"$(FLASH_TOOL)" -c port=SWD -w "$<" -v -rst

clean:
	rm -rf Output

-include $(OBJECTS:.o=.d)
