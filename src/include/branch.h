// src/include/branch.h

#pragma once
#include <stdint.h>

void handle_b(uint32_t instr);
void handle_bl(uint32_t instr);
void handle_bx(uint32_t instr);
void handle_blx_reg(uint32_t instr);