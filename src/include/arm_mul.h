// include/arm_mul.h
#pragma once
#include <stdint.h>

void handle_umull(uint32_t instr);
void handle_umlal(uint32_t instr);
void handle_smull(uint32_t instr);
void handle_smlal(uint32_t instr);