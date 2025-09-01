// src/include/logic.h
#pragma once
#include <stdint.h>

// Data-processing "logic" fast paths and immediates.
// All handlers take the raw 32-bit instruction and return void.

void handle_orr(uint32_t instr);
void handle_bic(uint32_t instr);

void handle_and_imm_dp(uint32_t instr);
void handle_and_reg_simple(uint32_t instr);

void handle_tst_imm(uint32_t instr);
void handle_tst_reg(uint32_t instr);

void handle_cmp_reg(uint32_t instr);
void handle_cmp_imm(uint32_t instr);

void handle_mov(uint32_t instr);     // DP MOV (register or immediate via dp_operand2)
void handle_rsb_imm(uint32_t instr);
void handle_cmn_imm(uint32_t instr);

void handle_movw(uint32_t instr);    // MOVW (imm16 -> low half; upper half zeroed)
void handle_movt(uint32_t instr);    // MOVT (imm16 -> upper half; low half preserved)
void handle_mov_imm(uint32_t instr); // DP MOV immediate fast path (same semantics as handle_mov)
