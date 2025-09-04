AS = arm-none-eabi-as
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

BASE = test_teq

SRC = $(BASE).s
OBJ = $(BASE).o
ELF = $(BASE).elf
BIN = $(BASE).bin
LDS = linker.ld

all: $(BIN)

$(OBJ): $(SRC)
	$(AS) -mcpu=cortex-a15 -o $@ $<

$(ELF): $(OBJ) $(LDS)
	$(LD) -T $(LDS) -o $@ $(OBJ)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin *.log

test:
	python run_tests.py

objdump:
	$(OBJDUMP) -d $(ELF)