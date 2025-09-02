// src/arm_mul.c
#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"       // CPU cpu, CPSR bits
#include "cond.h"      // evaluate_condition(...)
#include "arm_mul.h"

#ifndef BIT
#define BIT(x) (1u << (x))
#endif

// Only set N/Z; leave C/V unchanged for these long-multiply ops.
static inline void set_nz_64(uint64_t val) {
    // Z = 1 if full 64-bit result is zero
    if (val == 0) cpu.cpsr |= BIT(30); else cpu.cpsr &= ~BIT(30);
    // N = bit[63] of the 64-bit result
    if (val & 0x8000000000000000ULL) cpu.cpsr |= BIT(31); else cpu.cpsr &= ~BIT(31);
}

static inline bool mul_unpredictable_pc(uint32_t RdHi, uint32_t RdLo,
                                        uint32_t Rm, uint32_t Rs) {
    return (RdHi == 15u) | (RdLo == 15u) | (Rm == 15u) | (Rs == 15u);
}

// UMULL{S} RdLo,RdHi,Rm,Rs
void handle_umull(uint32_t instr) {
    uint32_t cond = (instr >> 28) & 0xFu;
    if (cond != 0xFu && !evaluate_condition(cond)) return;

    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    uint64_t res = (uint64_t)cpu.r[Rm] * (uint64_t)cpu.r[Rs];
    cpu.r[RdLo]  = (uint32_t)res;
    cpu.r[RdHi]  = (uint32_t)(res >> 32);
    if (S) set_nz_64(res);
}

// UMLAL{S} RdLo,RdHi,Rm,Rs
void handle_umlal(uint32_t instr) {
    uint32_t cond = (instr >> 28) & 0xFu;
    if (cond != 0xFu && !evaluate_condition(cond)) return;

    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    uint64_t acc = ((uint64_t)cpu.r[RdHi] << 32) | cpu.r[RdLo];
    uint64_t res = acc + (uint64_t)cpu.r[Rm] * (uint64_t)cpu.r[Rs];
    cpu.r[RdLo]  = (uint32_t)res;
    cpu.r[RdHi]  = (uint32_t)(res >> 32);
    if (S) set_nz_64(res);
}

// SMULL{S} RdLo,RdHi,Rm,Rs
void handle_smull(uint32_t instr) {
    uint32_t cond = (instr >> 28) & 0xFu;
    if (cond != 0xFu && !evaluate_condition(cond)) return;

    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    int64_t res = (int64_t)(int32_t)cpu.r[Rm] * (int64_t)(int32_t)cpu.r[Rs];
    cpu.r[RdLo] = (uint32_t)res;
    cpu.r[RdHi] = (uint32_t)((uint64_t)res >> 32);
    if (S) set_nz_64((uint64_t)res);
}

// SMLAL{S} RdLo,RdHi,Rm,Rs
void handle_smlal(uint32_t instr) {
    uint32_t cond = (instr >> 28) & 0xFu;
    if (cond != 0xFu && !evaluate_condition(cond)) return;

    uint32_t S    = (instr >> 20) & 1u;
    uint32_t RdHi = (instr >> 16) & 0xFu;
    uint32_t RdLo = (instr >> 12) & 0xFu;
    uint32_t Rs   = (instr >> 8)  & 0xFu;
    uint32_t Rm   =  instr        & 0xFu;

    if (mul_unpredictable_pc(RdHi, RdLo, Rm, Rs)) return;

    int64_t acc = ((int64_t)(int32_t)cpu.r[RdHi] << 32) | cpu.r[RdLo];
    int64_t res = acc + (int64_t)(int32_t)cpu.r[Rm] * (int64_t)(int32_t)cpu.r[Rs];
    cpu.r[RdLo] = (uint32_t)res;
    cpu.r[RdHi] = (uint32_t)((uint64_t)res >> 32);
    if (S) set_nz_64((uint64_t)res);
}

void handle_mul(uint32_t instr) {
    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rd = (instr >> 16) & 0xFu;    // Rd
    uint32_t Rn = (instr >> 12) & 0xFu;    // must be 0 in MUL encoding
    uint32_t Rs = (instr >> 8)  & 0xFu;    // Rs
    uint32_t Rm =  instr        & 0xFu;    // Rm

    (void)Rn; // MUL ignores Rn (architecturally 0000)

    uint32_t res = cpu.r[Rm] * cpu.r[Rs];
    cpu.r[Rd] = res;

    if (S) {
        cpu.cpsr = (cpu.cpsr & 0x3FFFFFFFu) |
                   (res & 0x80000000u)      |      // N
                   ((res == 0) ? 0x40000000u : 0); // Z
        // C and V are UNPREDICTABLE for MUL/MLA — leave unchanged
    }
}

void handle_mla(uint32_t instr) {
    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rd = (instr >> 16) & 0xFu;    // Rd
    uint32_t Rn = (instr >> 12) & 0xFu;    // Rn (accumulate)
    uint32_t Rs = (instr >> 8)  & 0xFu;    // Rs
    uint32_t Rm =  instr        & 0xFu;    // Rm

    uint32_t res = cpu.r[Rm] * cpu.r[Rs] + cpu.r[Rn];
    cpu.r[Rd] = res;

    if (S) {
        cpu.cpsr = (cpu.cpsr & 0x3FFFFFFFu) |
                   (res & 0x80000000u) |
                   ((res == 0) ? 0x40000000u : 0);
    }
}
