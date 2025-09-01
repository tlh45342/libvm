# Makefile — bare-metal UART "Hello World" (single C file, no linker script)

TARGET   := putchars_uart_c
SRC      := $(TARGET).c
ELF      := $(TARGET).elf
BIN      := $(TARGET).bin
LST      := $(TARGET).lst
MAP      := $(TARGET).map
SYM      := $(TARGET).sym
ASM      := $(TARGET).s

# Allow overriding tool prefix: make CROSS=arm-none-eabi-
CROSS    ?= arm-none-eabi-

CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
NM       := $(CROSS)nm

CFLAGS   := -nostdlib -nostartfiles -ffreestanding -fno-builtin \
            -mcpu=cortex-a15 -O2 -Wall -Wextra -Werror
LDFLAGS  := -Wl,-Ttext=0x8000 -Wl,-e,_start -Wl,-Map=$(MAP)

.PHONY: all clean disasm symbols hexdump code

all: $(BIN)

# Build ELF directly from the single C file
$(ELF): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# Raw binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Disassembly (mixed source/asm)
disasm: $(ELF)
	$(OBJDUMP) -d -S $< > $(LST)

# Symbols sorted by address
symbols: $(ELF)
	$(NM) -n $< > $(SYM)

# View hex (requires 'hexdump' on PATH)
hexdump: $(BIN)
	hexdump $(BIN)

# Emit compiler-generated assembly (for curiosity)
code: $(SRC)
	$(CC) $(CFLAGS) -S -o $(ASM) $<

# Nuke all build artifacts
clean:
	rm -f $(ELF) $(BIN) $(MAP) $(LST) $(SYM) $(ASM) *.o *.log
	
test:
	python run_tests.py