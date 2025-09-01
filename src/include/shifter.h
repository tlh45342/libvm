#pragma once
#include <stdint.h>
#include "cpu.h"
#include "cpu_flags.h"

extern CPU cpu;

static inline uint32_t ror32(uint32_t v, unsigned r) {
    r &= 31u; return r ? ((v >> r) | (v << (32 - r))) : v;
}

// Returns value; stores shifter carry-out to *sh_carry.
uint32_t dp_operand2(uint32_t instr, uint32_t *sh_carry);