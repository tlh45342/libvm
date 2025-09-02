// src/include/system.h
#pragma once
#include <stdint.h>

void handle_svc(uint32_t instr);
void handle_mrs(uint32_t instr);
void handle_msr(uint32_t instr);
void handle_msr_reg(uint32_t instr);
void handle_msr_imm(uint32_t instr);
void handle_setend(uint32_t instr);
void handle_cps(uint32_t instr);
void handle_dsb(uint32_t instr);
void handle_dmb(uint32_t instr);
void handle_isb(uint32_t instr);
void handle_nop(uint32_t instr);
void handle_wfi(uint32_t instr);
void handle_bkpt(uint32_t instr);
void handle_deadbeef(uint32_t instr);
void handle_bfc(uint32_t instr);
void handle_bfi(uint32_t instr);
void handle_clz(uint32_t instr);
void handle_msr(uint32_t instr);