// src/alu.c
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_flags.h"
#include "alu.h"
#include "operand.h"   // arm_read_src_reg(...)

// -----------------------------------------------------------------------------
// Helpers for PC writes under NPC model
// -----------------------------------------------------------------------------
static inline void write_pc_or_npc(uint32_t value, bool s_bit)
{
    if (s_bit) {
        // Data-processing with S=1 and Rd==PC => exception return semantics
        // (CPSR := SPSR_<mode>, PC := value)
        cpu_exception_return(value);
    } else {
        // Plain branch via DP write to PC (ARM state keeps T unchanged)
        // Word align (bits[1:0] cleared).
        cpu.npc = value & ~3u;
    }
}

// -----------------------------------------------------------------------------
// ADD / ADDS  Rd = Rn + op2
// -----------------------------------------------------------------------------
void handle_add(uint32_t instr)
{
    const int     rn = (instr >> 16) & 0xF;
    const int     rd = (instr >> 12) & 0xF;
    const bool    S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();          // dp_operand2 needs an input ref
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t op1 = arm_read_src_reg(rn);

    const uint64_t wide = (uint64_t)op1 + (uint64_t)op2;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        const uint32_t N = res >> 31;
        const uint32_t Z = (res == 0);
        const uint32_t C = (uint32_t)(wide >> 32);
        const uint32_t V = (~(op1 ^ op2) & (op1 ^ res)) >> 31;
        cpu.cpsr = (cpu.cpsr & ~((1u<<31)|(1u<<30)|(1u<<29)|(1u<<28)))
                 | (N<<31) | (Z<<30) | (C<<29) | (V<<28);
    }
}

// -----------------------------------------------------------------------------
// ADC / ADCS  Rd = Rn + op2 + C
// -----------------------------------------------------------------------------
void handle_adc(uint32_t instr)
{
    const int     rn = (instr >> 16) & 0xF;
    const int     rd = (instr >> 12) & 0xF;
    const bool    S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t op1 = arm_read_src_reg(rn);
    const uint32_t cin = cpsr_get_C();

    const uint64_t wide = (uint64_t)op1 + (uint64_t)op2 + (uint64_t)cin;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from((wide >> 32) & 1u);
        const uint32_t b = op2 + cin;
        const int overflow = (~(op1 ^ b) & (op1 ^ res) & 0x80000000u) != 0;
        cpsr_set_V(overflow);
    }
}

// -----------------------------------------------------------------------------
// SUB / SUBS  Rd = Rn - op2
// -----------------------------------------------------------------------------
void handle_sub(uint32_t instr)
{
    const int     rn = (instr >> 16) & 0xF;
    const int     rd = (instr >> 12) & 0xF;
    const bool    S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint32_t res = a - op2;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(a >= op2); // NOT borrow
        const int overflow = ((a ^ op2) & (a ^ res) & 0x80000000u) != 0;
        cpsr_set_V(overflow);
    }
}

// -----------------------------------------------------------------------------
// SBC / SBCS  Rd = Rn - op2 - (1 - C)
// -----------------------------------------------------------------------------
void handle_sbc(uint32_t instr)
{
    const int     rn = (instr >> 16) & 0xF;
    const int     rd = (instr >> 12) & 0xF;
    const bool    S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2    = dp_operand2(instr, &sh_carry);
    const uint32_t a      = arm_read_src_reg(rn);
    const uint32_t cin    = cpsr_get_C();
    const uint32_t borrow = 1u - (cin & 1u);

    const uint64_t wide = (uint64_t)a - (uint64_t)op2 - (uint64_t)borrow;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from((wide >> 32) == 0); // no borrow => C=1
        const uint32_t btot = op2 + borrow;
        const int overflow = ((a ^ btot) & (a ^ res) & 0x80000000u) != 0;
        cpsr_set_V(overflow);
    }
}

// -----------------------------------------------------------------------------
// RSB / RSBS  Rd = op2 - Rn
// -----------------------------------------------------------------------------
void handle_rsb(uint32_t instr)
{
    const int     rn = (instr >> 16) & 0xF;
    const int     rd = (instr >> 12) & 0xF;
    const bool    S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint32_t res = op2 - a;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(op2 >= a);
        const int overflow = ((op2 ^ a) & (op2 ^ res) & 0x80000000u) != 0;
        cpsr_set_V(overflow);
    }
}

// -----------------------------------------------------------------------------
// RSC / RSCS  Rd = op2 - Rn - (1 - C)
// -----------------------------------------------------------------------------
void handle_rsc(uint32_t instr)
{
    const int     rn = (instr >> 16) & 0xF;
    const int     rd = (instr >> 12) & 0xF;
    const bool    S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2    = dp_operand2(instr, &sh_carry);
    const uint32_t a      = arm_read_src_reg(rn);
    const uint32_t cin    = cpsr_get_C();
    const uint32_t borrow = 1u - (cin & 1u);

    const uint64_t wide = (uint64_t)op2 - (uint64_t)a - (uint64_t)borrow;
    const uint32_t res  = (uint32_t)wide;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from((wide >> 32) == 0);
        const uint32_t atot = a + borrow;
        const int overflow = ((op2 ^ atot) & (op2 ^ res) & 0x80000000u) != 0;
        cpsr_set_V(overflow);
    }
}

// -----------------------------------------------------------------------------
// AND / ANDS
// -----------------------------------------------------------------------------
void handle_and(uint32_t instr)
{
    const int   rn = (instr >> 16) & 0xF;
    const int   rd = (instr >> 12) & 0xF;
    const bool  S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) & op2;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(sh_carry & 1u);
    }
}

// -----------------------------------------------------------------------------
// EOR / EORS
// -----------------------------------------------------------------------------
void handle_eor(uint32_t instr)
{
    const int   rn = (instr >> 16) & 0xF;
    const int   rd = (instr >> 12) & 0xF;
    const bool  S  = ((instr >> 20) & 1u) != 0;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) ^ op2;

    if (rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(sh_carry & 1u);
    }
}

// -----------------------------------------------------------------------------
// MOV / MOVS  (DP form; immediate or reg via dp_operand2)
// -----------------------------------------------------------------------------
void handle_mov_dp(uint32_t instr)
{
    const bool S  = ((instr >> 20) & 1u) != 0;
    const int  Rd = (instr >> 12) & 0xF;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = op2;

    if (Rd == 15) { write_pc_or_npc(res, S); return; }

    cpu.r[Rd] = res;
    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(sh_carry & 1u); // MOVS: C := shifter carry
    }
}

// -----------------------------------------------------------------------------
// MVN / MVNS
// -----------------------------------------------------------------------------
void handle_mvn(uint32_t instr)
{
    const bool S  = ((instr >> 20) & 1u) != 0;
    const int  Rd = (instr >> 12) & 0xF;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = ~op2;

    if (Rd == 15) { write_pc_or_npc(res, S); return; }
    cpu.r[Rd] = res;

    if (S) {
        cpsr_set_NZ(res);
        cpsr_set_C_from(sh_carry & 1u);
    }
}

// -----------------------------------------------------------------------------
// CMP  (sets flags only)  Rn - op2
// -----------------------------------------------------------------------------
void handle_cmp(uint32_t instr)
{
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint32_t res = a - op2;

    cpsr_set_NZ(res);
    cpsr_set_C_from(a >= op2);
    const int overflow = ((a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    cpsr_set_V(overflow);
}

// -----------------------------------------------------------------------------
// TST  (sets N,Z and C from shifter) : (Rn & op2)
// -----------------------------------------------------------------------------
void handle_tst(uint32_t instr)
{
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) & op2;

    cpsr_set_NZ(res);
    cpsr_set_C_from(sh_carry & 1u);
}

// -----------------------------------------------------------------------------
// TEQ  (sets N,Z and C from shifter) : (Rn ^ op2)
// -----------------------------------------------------------------------------
void handle_teq(uint32_t instr)
{
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t res = arm_read_src_reg(rn) ^ op2;

    cpsr_set_NZ(res);
    cpsr_set_C_from(sh_carry & 1u);
}

// -----------------------------------------------------------------------------
// CMN  (sets flags only)  Rn + op2
// -----------------------------------------------------------------------------
void handle_cmn(uint32_t instr)
{
    const int rn = (instr >> 16) & 0xF;

    uint32_t sh_carry = cpsr_get_C();
    const uint32_t op2 = dp_operand2(instr, &sh_carry);
    const uint32_t a   = arm_read_src_reg(rn);
    const uint64_t sum = (uint64_t)a + (uint64_t)op2;
    const uint32_t res = (uint32_t)sum;

    cpsr_set_NZ(res);
    cpsr_set_C_from((sum >> 32) & 1u);
    const int overflow = (~(a ^ op2) & (a ^ res) & 0x80000000u) != 0;
    cpsr_set_V(overflow);
}
