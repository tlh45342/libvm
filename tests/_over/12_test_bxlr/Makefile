AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

SRC = test_bxlr.s
OBJ = test_bxlr.o
ELF = test_bxlr.elf
BIN = test_bxlr.bin
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