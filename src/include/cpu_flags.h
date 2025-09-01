#pragma once
#include <stdint.h>
#include "cpu.h"
#include "arm-vm.h"
#include "log.h"

extern CPU cpu;

static inline uint32_t cpsr_get_C(void) { return (cpu.cpsr >> 29) & 1u; }
static inline uint32_t cpsr_get_V(void) { return (cpu.cpsr >> 28) & 1u; }

static inline void cpsr_set_NZ(uint32_t result) {
    if (result & 0x80000000u) cpu.cpsr |=  BIT(31); else cpu.cpsr &= ~BIT(31);
    if (result == 0)           cpu.cpsr |=  BIT(30); else cpu.cpsr &= ~BIT(30);
}
static inline void cpsr_set_C_from(uint32_t c) { if (c) cpu.cpsr |= BIT(29); else cpu.cpsr &= ~BIT(29); }
static inline void cpsr_set_V(uint32_t v)      { if (v) cpu.cpsr |= BIT(28); else cpu.cpsr &= ~BIT(28); }
static inline uint32_t cpsr_mode(void){ return cpu.cpsr & CPSR_MODE_MASK; }
static inline int is_user_mode(void){ return (cpsr_mode() == 0x10u); }

// --- Inline CPSR flag helpers (linker-safe) ---------------------------------
#include "cpu.h"   // needs CPU cpu and CPSR_* masks

static inline void cpu_set_flag_N(bool v) {
    if (v) cpu.cpsr |=  CPSR_N; else cpu.cpsr &= ~CPSR_N;
}
static inline void cpu_set_flag_Z(bool v) {
    if (v) cpu.cpsr |=  CPSR_Z; else cpu.cpsr &= ~CPSR_Z;
}
static inline void cpu_set_flag_C(bool v) {
    if (v) cpu.cpsr |=  CPSR_C; else cpu.cpsr &= ~CPSR_C;
}
static inline void cpu_set_flag_V(bool v) {
    if (v) cpu.cpsr |=  CPSR_V; else cpu.cpsr &= ~CPSR_V;
}
void psr_write(uint32_t *psr, uint32_t value, uint32_t fields, int is_cpsr);