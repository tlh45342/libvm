// src/cpu/logic.c

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_flags.h"
#include "cond.h"
#include "logic.h"
#include "operand.h"   // <-- needed for dp_operand2()

// ------------------------------- ORR -------------------------------
// Rd = Rn | op2 ; flags (N,Z,C from shifter) if S=1
void handle_orr(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = arm_read_src_reg(Rn) | op2;

    if (Rd == 15u) {
        if (S) cpu_exception_return(res);
        else   cpu.npc = res & ~1u;
        return;
    }

    cpu.r[Rd] = res;
    if (S) { cpsr_set_NZ(res); cpsr_set_C_from(sh_carry & 1u); }
}

// ------------------------------- BIC -------------------------------
// Rd = Rn & ~op2 ; flags if S=1
void handle_bic(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = arm_read_src_reg(Rn) & ~op2;

    if (Rd == 15u) {
        if (S) cpu_exception_return(res);
        else   cpu.npc = res & ~1u;
        return;
    }

    cpu.r[Rd] = res;
    if (S) { cpsr_set_NZ(res); cpsr_set_C_from(sh_carry & 1u); }
}

// ------------------------------- AND imm/reg fast paths -------------------------------
// Rd = Rn & op2 ; flags if S=1
void handle_and_imm_dp(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = arm_read_src_reg(Rn) & op2;

    if (Rd == 15u) {
        if (S) cpu_exception_return(res);
        else   cpu.npc = res & ~1u;
        return;
    }

    cpu.r[Rd] = res;
    if (S) { cpsr_set_NZ(res); cpsr_set_C_from(sh_carry & 1u); }
}

void handle_and_reg_simple(uint32_t instr) {
    handle_and_imm_dp(instr);
}

// ------------------------------- TST imm/reg -------------------------------
// flags = Rn & op2 (always set flags; no writeback)
void handle_tst_imm(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rn = (instr >> 16) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t res = arm_read_src_reg(Rn) & op2;

    cpsr_set_NZ(res);
    cpsr_set_C_from(sh_carry & 1u);
}

void handle_tst_reg(uint32_t instr) {
    handle_tst_imm(instr);
}

// ------------------------------- CMP reg/imm -------------------------------
// flags = Rn - op2 (always set flags; no writeback)
void handle_cmp_reg(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rn = (instr >> 16) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t a   = arm_read_src_reg(Rn);

    uint32_t res = a - op2;
    cpsr_set_NZ(res);
    cpsr_set_C_from(a >= op2); // NOT borrow
    int overflow = ((a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    cpsr_set_V(overflow);
}

void handle_cmp_imm(uint32_t instr) {
    handle_cmp_reg(instr);
}

// ------------------------------- MOV (DP; reg or imm via dp_operand2) -------------------------------
// Rd = op2 ; flags if S=1
void handle_mov(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t res = dp_operand2(instr, &sh_carry);

    if (Rd == 15u) {
        if (S) cpu_exception_return(res);
        else   cpu.npc = res & ~1u;
        return;
    }

    cpu.r[Rd] = res;
    if (S) { cpsr_set_NZ(res); cpsr_set_C_from(sh_carry & 1u); }
}

// ------------------------------- RSB (imm fast path) -------------------------------
// Rd = op2 - Rn ; flags if S=1
void handle_rsb_imm(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t S  = (instr >> 20) & 1u;
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t a   = arm_read_src_reg(Rn);

    uint32_t res = op2 - a;

    if (Rd == 15u) {
        if (S) cpu_exception_return(res);
        else   cpu.npc = res & ~1u;
        return;
    }

    cpu.r[Rd] = res;
    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(op2 >= a); // NOT borrow
        int overflow = ((op2 ^ a) & (op2 ^ res) & 0x80000000u) != 0;
        cpsr_set_V(overflow);
    }
}

// ------------------------------- CMN (imm fast path) -------------------------------
// flags = Rn + op2 (always set flags)
void handle_cmn_imm(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rn = (instr >> 16) & 0xFu;

    uint32_t sh_carry = cpsr_get_C();
    uint32_t op2 = dp_operand2(instr, &sh_carry);
    uint32_t a   = arm_read_src_reg(Rn);

    uint32_t res = a + op2;
    cpsr_set_NZ(res);
    cpsr_set_C_from(res < a); // carry out
    int overflow = (~(a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    cpsr_set_V(overflow);
}

// ------------------------------- MOVW / MOVT / MOV(imm fast path) -------------------------------
// MOVW: Rd = imm16 (zero-extend)
void handle_movw(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rd    = (instr >> 12) & 0xFu;
    uint32_t imm4  = (instr >> 16) & 0xFu;      // bits 19:16
    uint32_t imm12 =  instr        & 0xFFFu;    // bits 11:0
    uint32_t res   = (imm4 << 12) | imm12;      // zero-extended 16-bit

    if (Rd == 15u) {
        cpu.npc = res & ~1u;
        return;
    }
    cpu.r[Rd] = res;
}

// MOVT: Rd[31:16] = imm16 ; Rd[15:0] preserved
void handle_movt(uint32_t instr) {
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rd    = (instr >> 12) & 0xFu;
    uint32_t imm4  = (instr >> 16) & 0xFu;
    uint32_t imm12 =  instr        & 0xFFFu;
    uint32_t imm16 = (imm4 << 12) | imm12;

    uint32_t base  = cpu.r[Rd] & 0x0000FFFFu;
    uint32_t res   = base | (imm16 << 16);

    if (Rd == 15u) {
        cpu.npc = res & ~1u;
        return;
    }
    cpu.r[Rd] = res;
}

// MOV (imm fast path): identical semantics to DP MOV
void handle_mov_imm(uint32_t instr) {
    handle_mov(instr);
}

// Rd==PC is UNPREDICTABLE for these; keep VM robust by ignoring the write.
#define IGNORE_IF_PC(rd) do { if ((rd) == 15u) return; } while(0)

// ---------------- Byte/half reversals ----------------
void handle_rev(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    uint32_t x = cpu.r[Rm];
#if defined(__GNUC__) || defined(__clang__)
    cpu.r[Rd] = __builtin_bswap32(x);
#else
    cpu.r[Rd] = (x >> 24) | ((x >> 8) & 0x0000FF00u)
              | ((x << 8) & 0x00FF0000u) | (x << 24);
#endif
}

void handle_rev16(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    uint32_t x = cpu.r[Rm];
    cpu.r[Rd] = ((x & 0x00FF00FFu) << 8) | ((x & 0xFF00FF00u) >> 8);
}

void handle_revsh(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    uint32_t x = cpu.r[Rm];
    uint32_t low = ((x & 0xFFu) << 8) | ((x >> 8) & 0xFFu);
    cpu.r[Rd] = (uint32_t)(int32_t)(int16_t)low;  // sign-extend
}

// ---------------- Base extend (no rotate) ----------------
void handle_uxtb(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    cpu.r[Rd] = cpu.r[Rm] & 0xFFu;
}

void handle_uxth(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    cpu.r[Rd] = cpu.r[Rm] & 0xFFFFu;
}

void handle_sxtb(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    cpu.r[Rd] = (uint32_t)(int32_t)(int8_t)(cpu.r[Rm] & 0xFFu);
}

void handle_sxth(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;
    IGNORE_IF_PC(Rd);
    cpu.r[Rd] = (uint32_t)(int32_t)(int16_t)(cpu.r[Rm] & 0xFFFFu);
}

// helper (not exported, not in table)
static inline void exec_mov_common(uint32_t instr, uint32_t Rd, int Sbit) {
    uint32_t sh_c = cpsr_get_C();
    uint32_t op2  = dp_operand2(instr, &sh_c);
    cpu.r[Rd] = op2;
    if (Sbit) {
        cpsr_set_NZ(op2);
        cpsr_set_C_from(sh_c & 1u);
    }
}

// One helper for both MOV/MVN; not exported, not referenced in the table directly.
static inline void exec_mov_or_mvn_common(uint32_t instr, uint32_t Rd, int Sbit, bool is_mvn) {
    // Seed shifter carry-in from CPSR.C
    uint32_t sh_c = cpsr_get_C();

    // Compute Operand2; dp_operand2 updates sh_c to the shifter carry-out if defined
    uint32_t op2  = dp_operand2(instr, &sh_c);
    uint32_t res  = is_mvn ? ~op2 : op2;

    // Special PC write semantics
    if (Rd == 15u) {
        if (Sbit) {
            // MOVS/MVNS to PC => exception-return semantics (restores CPSR/SPSR etc.)
            cpu_exception_return(res);
        } else {
            // MOV/MVN to PC => branch; clear T in npc alignment
            cpu.npc = res & ~1u;
        }
        return;
    }

    // Normal register write
    cpu.r[Rd] = res;

    // Flag updates for S variants: N/Z from result; C from shifter carry-out
    if (Sbit) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(sh_c & 1u);  // keep consistent with the rest of this file
    }
}

// MOV (data-processing form: reg/imm via dp_operand2)
void handler_mov(uint32_t instr) {
    // Honor condition codes (treat as NOP if condition fails)
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rd = (instr >> 12) & 0xF;
    int Sbit    = (instr >> 20) & 1;

    exec_mov_or_mvn_common(instr, Rd, Sbit, /*is_mvn=*/false);
}

// MVN (data-processing form)
void handler_mvn(uint32_t instr) {
    // Honor condition codes (treat as NOP if condition fails)
    uint8_t cond = (instr >> 28) & 0xF;
    if (cond != 0xF && !evaluate_condition(cond)) return;

    uint32_t Rd = (instr >> 12) & 0xF;
    int Sbit    = (instr >> 20) & 1;

    exec_mov_or_mvn_common(instr, Rd, Sbit, /*is_mvn=*/true);
}