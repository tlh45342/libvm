#pragma once
#include <stdint.h>

void handle_ldr_preimm(uint32_t instr);
void handle_str_preimm(uint32_t instr);
void handle_str_predec(uint32_t instr);
void handle_ldr_postimm(uint32_t instr);
void handle_str_postimm(uint32_t instr);
void handle_ldr_regoffset(uint32_t instr);
void handle_ldrb_preimm(uint32_t instr);
void handle_ldrb_postimm(uint32_t instr);
void handle_ldrb_reg(uint32_t instr);
void handle_ldrb_reg_shift(uint32_t instr);
void handle_strb_preimm(uint32_t instr);
void handle_strb_postimm(uint32_t instr);
void handle_ldr_literal(uint32_t instr);
void handle_ldm(uint32_t instr);
void handle_stm(uint32_t instr);
void exec_ldrd_imm(uint32_t instr);
void exec_ldrd_reg(uint32_t instr);
void exec_strd_imm(uint32_t instr);
void exec_strd_reg(uint32_t instr);
void handle_pop_pc(uint32_t instr);
void handle_strh (uint32_t instr);
void handle_ldrh (uint32_t instr);
void handle_ldrsb(uint32_t instr);
void handle_ldrsh(uint32_t instr);
void handle_ldm(uint32_t instr);
void handle_stm(uint32_t instr);
void handle_strh (uint32_t instr);
void handle_ldrh (uint32_t instr);
void handle_ldrsb(uint32_t instr);
void handle_ldrsh(uint32_t instr);
void handle_str_regoffset(uint32_t instr);
void handle_strb_reg_shift(uint32_t instr);
void handle_swp(uint32_t instr);
void handle_swpb(uint32_t instr);
