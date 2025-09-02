// src/cpu/branch.c
#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "cond.h"   // evaluate_condition()

// Sign-extend imm24<<2 in one go.
static inline int32_t br_off_imm24(uint32_t instr) {
    int32_t imm = (int32_t)(instr & 0x00FFFFFFu);
    return (imm << 8) >> 6;   // == sign_extend((imm24<<2), 26)
}

// ---- B (Branch) ----
// If the condition passes, set npc to target. Otherwise leave npc as the
// fall-through (stepper pre-initializes npc = pc + 4).
void handle_b(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t A   = cpu.r[15];
    int32_t  off = br_off_imm24(instr);
    cpu.npc      = (A + 8) + (uint32_t)off;  // ARM state: PC reads as A+8
}

// ---- BL (Branch with Link) ----
void handle_bl(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t A   = cpu.r[15];
    int32_t  off = br_off_imm24(instr);

    cpu.r[14] = A + 4;                           // architectural LR
    cpu.npc   = (A + 8) + (uint32_t)off;
}

// ---- BX (Branch and Exchange) ----
// Thumb not modeled yet: just clear bit0 and stay in ARM state.
void handle_bx(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t rm  = instr & 0xF;
    uint32_t tgt = cpu.r[rm];
    cpu.npc      = tgt & ~1u;                    // align to ARM
}

void handle_blx_reg(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;
    uint32_t A = cpu.r[15];
    uint32_t tgt = cpu.r[instr & 0xF];
    cpu.r[14] = A + 4;
    cpu.npc   = tgt & ~1u;   // stay ARM
}

// ---- BLX (Branch with Link, immediate form) ----
// Unconditional (cond=1111 in encoding). ARM-only VM: ignore Thumb request; clear bit0.
// Encoding: 1111 101H imm24
// Target = (A + 8) + sign_extend_26( (imm24 << 2) | (H << 1) )
// LR = A + 4
void handle_blx_imm(uint32_t instr) {
    uint32_t A     = cpu.r[15];                  // address of this instruction
    uint32_t imm24 =  instr        & 0x00FFFFFFu;
    uint32_t H     = (instr >> 24) & 0x1u;

    uint32_t off26 = (imm24 << 2) | (H << 1);    // build 26-bit offset
    int32_t  off   = (int32_t)(off26 << 6) >> 6; // sign-extend from 26 bits

    cpu.r[14] = A + 4;                           // LR = next instruction
    cpu.npc   = ((A + 8) + (uint32_t)off) & ~1u; // stay ARM: clear bit0
}