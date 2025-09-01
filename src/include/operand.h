// operand.h
#ifndef ARM_VM_OPERAND_H
#define ARM_VM_OPERAND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Expand Operand2 for ARM data-processing instructions.
// *carry_out must be initialized to the current CPSR.C before the call.
// On return, *carry_out is updated iff the shifter defines it.
uint32_t dp_operand2(uint32_t instr, uint32_t *carry_out);

// Source register read helper used by dp_operand2/ALU paths.
// Provide this in your project (e.g., in cpu.h / cpu.c).
#ifndef ARMVM_HAVE_arm_read_src_reg
uint32_t arm_read_src_reg(int r);
#endif

// Optional inline fallback (define ARMVM_OPERAND_INLINE_FALLBACK before including
// this header if you want a self-contained version that reads PC as PC+8).
#ifdef ARMVM_OPERAND_INLINE_FALLBACK
#include "cpu.h"
static inline uint32_t arm_read_src_reg(int r) {
    return (r == 15) ? (cpu.r[15] + 8u) : cpu.r[r];
}
#define ARMVM_HAVE_arm_read_src_reg 1
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ARM_VM_OPERAND_H
