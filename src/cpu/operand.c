// src/cpu/operand.c — canonical A32 Operand2 (shifter) implementation
// Implements immediate and register-specified shifts per ARM ARM.
// Critical edge cases covered:
// - LSL #0: value unchanged; carry unchanged
// - LSL #32 (imm): result=0;        C = Rm[0]
// - LSL >32 (imm): result=0;        C = 0
// - LSR #0  (imm): treated as #32;  result=0; C = Rm[31]
// - ASR #0  (imm): treated as #32;  result=all-sign; C = Rm[31]
// - RRX     (ROR imm #0): (C<<31)|(Rm>>1); C = Rm[0]
// - Reg-specified shifts use Rs[7:0]; amount==0 => value unchanged; carry unchanged
// - Reg-specified LSL ==32: result=0; C = Rm[0]
// - Reg-specified LSL  >32: result=0; C = 0
// - Reg-specified LSR ==32: result=0; C = Rm[31];  >32: result=0; C=0
// - Reg-specified ASR >=32: result=all-sign; C = Rm[31]
// - Reg-specified ROR: rot = amount & 31; rot==0 && amount!=0 => value unchanged; C = Rm[31]
//   (amount==0 => carry unchanged; amount!=0 and rot==0 => C=Rm[31])

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"        // arm_read_src_reg()
#include "cpu_flags.h"  // cpsr_get_C()
#include "operand.h"    // ror32() inline, dp_operand2() decl

static inline uint32_t ror32(uint32_t val, unsigned int rot) {
    return (val >> rot) | (val << (32 - rot));
}

uint32_t dp_operand2(uint32_t instr, uint32_t *carry_out)
{
    const uint32_t I = (instr >> 25) & 1u;

    // -------------------------
    // Immediate (I == 1): rotated immediate
    // op2 = ROR(imm8, 2*rot); if rot==0 => C unchanged; else C = bit31(op2)
    // -------------------------
    if (I) {
        const uint32_t imm8 =  instr        & 0xFFu;
        const uint32_t rot   = (instr >> 8) & 0xFu;     // rotate right by 2*rot
        if (rot == 0) {
            // No rotation => carry unchanged
            return imm8;
        } else {
            uint32_t val = ror32(imm8, rot * 2u);
            *carry_out = (val >> 31) & 1u;
            return val;
        }
    }

    // -------------------------
    // Register-sourced Operand2 (I == 0)
    // -------------------------
    const uint32_t rm    =  instr        & 0xFu;
    const uint32_t rmval = arm_read_src_reg((int)rm);   // PC reads as PC+8 in your CPU
    const uint32_t bit4  = (instr >> 4) & 1u;

    // -------- immediate shift (bit4 == 0) --------
    if (bit4 == 0) {
        const uint32_t stype = (instr >> 5) & 3u;    // 0 LSL, 1 LSR, 2 ASR, 3 ROR
        const uint32_t shimm = (instr >> 7) & 0x1Fu;

        switch (stype) {
        case 0: // LSL (imm)
            if (shimm == 0) {
                // value unchanged; carry unaffected
                return rmval;
            } else if (shimm < 32) {
                *carry_out = (rmval >> (32u - shimm)) & 1u;
                return rmval << shimm;
            } else if (shimm == 32) {
                *carry_out = rmval & 1u;     // *** C = bit0(Rm) ***
                return 0u;
            } else { // shimm > 32
                *carry_out = 0;
                return 0u;
            }

        case 1: // LSR (imm)
            if (shimm == 0) {
                // LSR #32
                *carry_out = (rmval >> 31) & 1u;
                return 0u;
            } else if (shimm < 32) {
                *carry_out = (rmval >> (shimm - 1)) & 1u;
                return rmval >> shimm;
            } else { // shimm >= 32
                *carry_out = (shimm == 32) ? ((rmval >> 31) & 1u) : 0u;
                return 0u;
            }

        case 2: // ASR (imm)
            if (shimm == 0) {
                // ASR #32
                *carry_out = (rmval >> 31) & 1u;
                return (rmval & 0x80000000u) ? 0xFFFFFFFFu : 0x00000000u;
            } else if (shimm < 32) {
                *carry_out = (rmval >> (shimm - 1)) & 1u;
                return (uint32_t)((int32_t)rmval >> shimm);
            } else { // shimm >= 32
                *carry_out = (rmval >> 31) & 1u;
                return (rmval & 0x80000000u) ? 0xFFFFFFFFu : 0x00000000u;
            }

        default: // 3: ROR (imm)   (RRX when shimm==0)
            if (shimm == 0) {
                // RRX: (C << 31) | (rm >> 1); C := bit0(rm)
                const uint32_t c = (*carry_out) & 1u;
                *carry_out = rmval & 1u;
                return (rmval >> 1) | (c << 31);
            } else {
                *carry_out = (rmval >> (shimm - 1)) & 1u;
                return ror32(rmval, shimm);
            }
        }
    }

    // -------- register-specified shift (bit4 == 1) --------
    {
        const uint32_t stype  = (instr >> 5) & 3u;    // 0 LSL, 1 LSR, 2 ASR, 3 ROR
        const uint32_t rs     = (instr >> 8) & 0xFu;
        const uint32_t amount = arm_read_src_reg((int)rs) & 0xFFu; // low 8 bits

        if (amount == 0) {
            // No shift; carry unaffected
            return rmval;
        }

        switch (stype) {
        case 0: // LSL (reg)
            if (amount < 32) {
                *carry_out = (rmval >> (32u - amount)) & 1u;
                return rmval << amount;
            } else if (amount == 32) {
                *carry_out = rmval & 1u;     // *** C = bit0(Rm) ***
                return 0u;
            } else { // amount > 32
                *carry_out = 0u;
                return 0u;
            }

        case 1: // LSR (reg)
            if (amount < 32) {
                *carry_out = (rmval >> (amount - 1)) & 1u;
                return rmval >> amount;
            } else if (amount == 32) {
                *carry_out = (rmval >> 31) & 1u;
                return 0u;
            } else { // amount > 32
                *carry_out = 0u;
                return 0u;
            }

        case 2: // ASR (reg)
            if (amount < 32) {
                *carry_out = (rmval >> (amount - 1)) & 1u;
                return (uint32_t)((int32_t)rmval >> amount);
            } else { // amount >= 32 => all sign bits; C = bit31
                *carry_out = (rmval >> 31) & 1u;
                return (rmval & 0x80000000u) ? 0xFFFFFFFFu : 0x00000000u;
            }

        default: // 3: ROR (reg)
        {
            const uint32_t rot = amount & 31u;
            if (rot == 0) {
                // amount != 0 and rot==0  => rotate by multiple of 32
                // result unchanged; C := Rm[31] (architecture-defined for this case)
                *carry_out = (rmval >> 31) & 1u;
                return rmval;
            } else {
                *carry_out = (rmval >> (rot - 1)) & 1u;
                return ror32(rmval, rot);
            }
        }
        }
    }
}