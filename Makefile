##############################################################################
# Makefile — Home Helper Device STM32F103C8T6
# Toolchain: arm-none-eabi-gcc
# HAL: STM32F1xx HAL (from STM32CubeF1)
##############################################################################

# ── Project ──────────────────────────────────────────────────────────────
PROJECT  = home_helper_stm32
MCU      = cortex-m3
TARGET   = STM32F103C8Tx

# ── Directories ───────────────────────────────────────────────────────────
CORE_SRC  = Core/Src
CORE_INC  = Core/Inc
HAL_SRC   = Drivers/STM32F1xx_HAL_Driver/Src
HAL_INC   = Drivers/STM32F1xx_HAL_Driver/Inc
CMSIS_INC = Drivers/CMSIS/Device/ST/STM32F1xx/Include \
            Drivers/CMSIS/Include
BUILD     = build

# ── Source files ──────────────────────────────────────────────────────────
# Application sources
APP_SRCS = $(wildcard $(CORE_SRC)/*.c)

# HAL sources (exclude template files)
HAL_SRCS = $(wildcard $(HAL_SRC)/*.c)
HAL_SRCS := $(filter-out %_template.c, $(HAL_SRCS))

# Startup assembly
STARTUP = startup_stm32f103c8tx.s

ALL_SRCS = $(APP_SRCS) $(HAL_SRCS) $(STARTUP)
OBJS     = $(patsubst %.c, $(BUILD)/%.o, $(APP_SRCS)) \
           $(patsubst %.c, $(BUILD)/hal_%.o, $(notdir $(HAL_SRCS))) \
           $(BUILD)/startup.o

# ── Linker script ─────────────────────────────────────────────────────────
LD_SCRIPT = STM32F103C8TX_FLASH.ld

# ── Toolchain ─────────────────────────────────────────────────────────────
CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb

# ── Compiler flags ────────────────────────────────────────────────────────
DEFS = -DSTM32F103xB -DUSE_HAL_DRIVER

INCS = $(addprefix -I, $(CORE_INC) $(HAL_INC) $(CMSIS_INC))

CFLAGS  = -mcpu=$(MCU) -mthumb
CFLAGS += $(DEFS) $(INCS)
CFLAGS += -Os -std=c11
CFLAGS += -Wall -Wextra -Wshadow -Wstrict-prototypes
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -g -gdwarf-2
CFLAGS += --specs=nano.specs

ASFLAGS = -mcpu=$(MCU) -mthumb -x assembler-with-cpp

LDFLAGS = -mcpu=$(MCU) -mthumb
LDFLAGS += -T$(LD_SCRIPT)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += --specs=nano.specs --specs=nosys.specs
LDFLAGS += -Wl,-Map=$(BUILD)/$(PROJECT).map,--cref

# ── Programmer (OpenOCD + ST-Link v2) ─────────────────────────────────────
OPENOCD       = openocd
OPENOCD_CFG   = -f interface/stlink.cfg -f target/stm32f1x.cfg

# ── Targets ───────────────────────────────────────────────────────────────
.PHONY: all clean flash size debug help

all: $(BUILD)/$(PROJECT).hex size

# Compile application sources
$(BUILD)/$(CORE_SRC)/%.o: $(CORE_SRC)/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	@echo "  CC  $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile HAL sources
$(BUILD)/hal_%.o: $(HAL_SRC)/%.c | $(BUILD)
	@echo "  CC  [HAL] $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup
$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	@echo "  AS  $<"
	$(AS) $(ASFLAGS) -c $< -o $@

# Link
$(BUILD)/$(PROJECT).elf: $(OBJS)
	@echo "  LD  $@"
	$(CC) $(LDFLAGS) $^ -o $@

# Convert to HEX
$(BUILD)/$(PROJECT).hex: $(BUILD)/$(PROJECT).elf
	@echo "  HEX $@"
	$(OBJCOPY) -O ihex $< $@

# Convert to BIN
$(BUILD)/$(PROJECT).bin: $(BUILD)/$(PROJECT).elf
	$(OBJCOPY) -O binary -S $< $@

$(BUILD):
	@mkdir -p $(BUILD)

size: $(BUILD)/$(PROJECT).elf
	$(SIZE) --format=berkeley $<

# Flash via OpenOCD + ST-Link
flash: $(BUILD)/$(PROJECT).bin
	$(OPENOCD) $(OPENOCD_CFG) \
	  -c "program $(BUILD)/$(PROJECT).bin verify reset exit 0x08000000"

# Launch GDB debug session
debug: $(BUILD)/$(PROJECT).elf
	$(OPENOCD) $(OPENOCD_CFG) &
	$(GDB) -ex "target remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load" $<

clean:
	@rm -rf $(BUILD)
	@echo "  CLEAN done"

help:
	@echo "Targets:"
	@echo "  make        — Build .elf and .hex"
	@echo "  make flash  — Flash via ST-Link + OpenOCD"
	@echo "  make size   — Show flash/RAM usage"
	@echo "  make debug  — Start GDB debug session"
	@echo "  make clean  — Remove build directory"
