# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Isrc/include
LDFLAGS =
LIBS    = -luser32 -lgdi32     # needed for wincrt.c (window + GDI)

# Paths
SRC_DIR     = src
CPU_DIR     = $(SRC_DIR)/cpu
HW_DIR      = $(SRC_DIR)/hw
EXE_FILE    = $(SRC_DIR)/arm-vm.exe
CRT_EXE     = $(SRC_DIR)/arm-vm-crt.exe
INSTALL_DIR = /cygdrive/c/cygwin64/bin

# ---- Core sources (non-CPU, non-HW) ----
SRCS_CORE = \
    $(SRC_DIR)/debug_globals.c \
    $(SRC_DIR)/mem.c \
    $(SRC_DIR)/dev_crt.c \
    $(SRC_DIR)/dev_disk.c \
    $(SRC_DIR)/cli.c \
    $(SRC_DIR)/vm.c \
    $(SRC_DIR)/log.c \
    $(SRC_DIR)/disasm.c \
    $(SRC_DIR)/wincrt.c \
    $(SRC_DIR)/disk_manager.c \
    $(SRC_DIR)/arm-vm.c

# ---- CPU sources (moved under src/cpu) ----
# Add execute.c here later if/when you split it out.
SRCS_CPU = \
    $(CPU_DIR)/system.c \
    $(CPU_DIR)/cond.c \
	$(CPU_DIR)/operand.c \
	$(CPU_DIR)/branch.c \
    $(CPU_DIR)/cpu_flags.c \
    $(CPU_DIR)/arm_mul.c \
	$(CPU_DIR)/logic.c \
	$(CPU_DIR)/cpu.c \
	$(CPU_DIR)/memops.c \
	$(CPU_DIR)/alu.c \
	$(CPU_DIR)/execute.c 

# ---- HW sources ----
SRCS_HW = \
    $(HW_DIR)/hw_bus.c \
    $(HW_DIR)/dev_uart.c \
    $(HW_DIR)/hw_disk.c

SRCS = $(SRCS_CORE) $(SRCS_CPU) $(SRCS_HW)
OBJS = $(SRCS:.c=.o)

TEST_DIRS := $(wildcard tests/*)

.PHONY: all clean install test crt

all: $(EXE_FILE)

$(EXE_FILE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

# Generic pattern rule (works in subdirs)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXE_FILE) $(CRT_EXE) $(SRC_DIR)/arm-vm-crt.o

install: $(EXE_FILE)
	cp $(EXE_FILE) $(INSTALL_DIR)/arm-vm.exe

# ---- CRT build ----
# Reuse all objects, but swap the main: arm-vm.c -> arm-vm-crt.c
# This avoids manually listing CPU objs again.
CRT_OBJS = $(filter-out $(SRC_DIR)/arm-vm.o,$(OBJS)) $(SRC_DIR)/arm-vm-crt.o

crt: $(CRT_EXE)

$(CRT_EXE): $(CRT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CRT_OBJS) $(LIBS) $(LDFLAGS)

test: all
	python run_tests.py

maxclean: clean
	@for d in $(TEST_DIRS); do \
	  if [ -d $$d ]; then \
	    echo "Cleaning $$d..."; \
	    $(MAKE) -C $$d clean; \
	  fi; \
	done