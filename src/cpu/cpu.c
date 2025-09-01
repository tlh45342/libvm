// src/cpu/cpu.c

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "cpu.h"
#include "mem.h"
#include "hw.h"
#include "execute.h"
#include "debug.h"    // debug_flags_t, trace_all
#include "log.h"      // only used by cpu_dump_registers()

extern bool trace_all;
extern debug_flags_t debug_flags;

CPU cpu = {0};
uint64_t cycle = 0;

static bool g_cpu_halted = false;

// -----------------------------------------------------------------------------
// Run-state control
// -----------------------------------------------------------------------------
void cpu_halt(void)       { g_cpu_halted = true; }
void cpu_clear_halt(void) { g_cpu_halted = false; }
bool cpu_is_halted(void)  { return g_cpu_halted; }

// -----------------------------------------------------------------------------
// Small utilities
// -----------------------------------------------------------------------------

// Architectural PC read for data processing/shifter paths.
// In ARM state, PC reads as (current PC + 8).
uint32_t arm_read_src_reg(int r) {
    r &= 15;
    uint32_t v = cpu.r[r];
    if (r == 15) v += 8u;  // ARM PC read semantics
    return v;
}

// Centralized fetch: sets cpu.npc to the fall-through (ARM: +4).
uint32_t cpu_fetch(void) {
    uint32_t pc = cpu.r[15];

    // This core runs A32 only; if T is set, normalize silently.
    if (cpu.cpsr & CPSR_T) {
        cpu.cpsr &= ~CPSR_T;
    }

#if defined(CPU_STRICT_FETCH)
    if (pc & 3u) {
        // Unaligned fetch → halt; return an undefined instruction pattern
        cpu_halt();
        return 0xE7F001F0u; // UDF
    }
#endif

    // Range check against currently bound memory
    size_t msz = mem_size();
    if (!mem_is_bound() || msz < 4 || pc > (uint32_t)(msz - 4)) {
        cpu_halt();
        return 0xDEADDEADu;
    }

    // Set fall-through next PC for this instruction
    cpu.npc = pc + 4u;   // ARM state

    // Fetch instruction
    return mem_read32(pc);
}

// Execute exactly one instruction: fetch → execute → commit
static int execute_one_instruction(void) {
    if (cpu_is_halted()) return 0;

    uint32_t instr = cpu_fetch();

    // Dispatch/execute (handlers may change cpu.npc)
    bool ok = execute(instr);

    // If we halted during execute (e.g., BKPT/DEADBEEF), do not commit PC.
    if (cpu_is_halted()) return 0;

    // Single commit point for control flow
    cpu.r[15] = cpu.npc;

    return ok ? 1 : 0;  // simple cycle accounting placeholder
}

void cpu_dump_registers(void) {
    // Convenience printer (only called explicitly)
    log_printf("Registers:\n");
    for (int i = 0; i < 16; i++) {
        log_printf("r%-2d = 0x%08x  ", i, cpu.r[i]);
        if ((i + 1) % 4 == 0) log_printf("\n");
    }
}

// -----------------------------------------------------------------------------
// Public stepping
// -----------------------------------------------------------------------------
void cpu_step(void) {
    int cycles_used = execute_one_instruction();
    if (cycles_used <= 0) cycles_used = 1;
    hw_bus_tick(cycles_used);
    // TODO: IRQ/FIQ sampling would go here later
}

// -----------------------------------------------------------------------------
// Exception return helpers
// -----------------------------------------------------------------------------

// If/when you add banked SPSRs for modes, route this through the bank.
static inline uint32_t cpu_get_spsr_current(void) {
    return cpu.spsr;
}

void cpu_exception_return(uint32_t new_pc) {
    // Restore CPSR and schedule the branch by writing NPC (not PC).
    cpu.cpsr = cpu_get_spsr_current();

    // If you later support Thumb, align based on CPSR.T before writing npc.
    // bool T = (cpu.cpsr >> 5) & 1u;
    // if (T) new_pc &= ~1u; else new_pc &= ~3u;

    cpu.npc = new_pc;
}
