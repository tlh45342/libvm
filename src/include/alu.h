#pragma once
#include <stdint.h>

// Data-processing (DP) instruction handlers
// Each function:
//   - evaluates the condition codes (and returns without side effects if not met)
//   - decodes S/Rn/Rd and Operand2 (via dp_operand2)
//   - performs the operation and updates flags/registers as appropriate

void handle_add(uint32_t instr);
void handle_eor(uint32_t instr);
void handle_sub(uint32_t instr);
void handle_rsb(uint32_t instr);
void handle_adc(uint32_t instr);
void handle_sbc(uint32_t instr);
void handle_rsc(uint32_t instr);

void handle_tst(uint32_t instr);  // flags only
void handle_teq(uint32_t instr);  // flags only
void handle_cmp(uint32_t instr);  // flags only
void handle_cmn(uint32_t instr);  // flags only

void handle_mvn(uint32_t instr);
void handle_and(uint32_t instr);
void handle_mov_dp(uint32_t instr);