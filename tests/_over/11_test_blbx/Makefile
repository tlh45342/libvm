AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

SRC = test_blbx.s
OBJ = test_blbx.o
ELF = test_blbx.elf
BIN = test_blbx.bin
LDS = linker.ld

all: $(BIN)

$(OBJ): $(SRC)
	$(AS) -mcpu=cortex-a15 -o $@ $<

$(ELF): $(OBJ) $(LDS)
	$(LD) -T $(LDS) -o $@ $(OBJ)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin
	
test:
	python run_tests.py