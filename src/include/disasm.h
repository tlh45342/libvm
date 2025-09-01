// disasm.h
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

void disasm_line(uint32_t pc, uint32_t instr, char *out, size_t out_sz);