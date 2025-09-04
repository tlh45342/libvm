// /src/cpu/cpu_flags.c

#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "cpu_flags.h"  // cpsr_set_NZ, cpsr_set_C_from, cpsr_get_C, is_user_mode
#include "log.h"
#include "cpu_flags.h"

extern debug_flags_t debug_flags;

void psr_write(uint32_t *psr, uint32_t value, uint32_t fields, int is_cpsr)
{
    uint32_t p = *psr;
    const uint32_t F = 1u<<3, S = 1u<<2, X = 1u<<1, C = 1u<<0;

    if (fields & C) {
        uint32_t keep = p & ~(CPSR_E|CPSR_A|CPSR_I|CPSR_F|CPSR_T|CPSR_MODE_MASK);
        uint32_t set  = value & (CPSR_E|CPSR_A|CPSR_I|CPSR_F|CPSR_MODE_MASK);
        p = keep | set;
    }
    if (fields & X) { /* ignored */ }
    if (fields & S) { p = (p & ~CPSR_GE_MASK) | (value & CPSR_GE_MASK); }
    if (fields & F) {
        uint32_t keep = p & ~(CPSR_N|CPSR_Z|CPSR_C|CPSR_V|CPSR_Q);
        uint32_t set  = value &  (CPSR_N|CPSR_Z|CPSR_C|CPSR_V|CPSR_Q);
        p = keep | set;
    }

    *psr = p;

    if (debug_flags & DBG_INSTR) {
        log_printf("[PSR write] %s <= 0x%08X (fields: %c%c%c%c) -> 0x%08X\n",
            is_cpsr ? "CPSR" : "SPSR", value,
            (fields&F)?'f':'-', (fields&S)?'s':'-', (fields&X)?'x':'-', (fields&C)?'c':'-',
            *psr);
    }
}
