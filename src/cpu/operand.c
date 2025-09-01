#include <stdint.h>
#include "cpu.h"
#include "operand.h"

// Rotate-right helper (n must be 1..31; caller handles 0/32 special cases)
static inline uint32_t ror32(uint32_t x, unsigned n) {
    n &= 31u;
    return (x >> n) | (x << (32u - n));
}

// -----------------------------------------------------------------------------
// dp_operand2
//   - Returns the shifter operand value for ARM data-processing instructions.
//   - *carry_out is both an input (current CPSR C, used by RRX) and an output
//     (updated shifter carry when the shift defines it; otherwise left unchanged).
//
// Semantics implemented per ARM ARM:
//   Immediate (I=1):
//     op2 = ROR(imm8, 2*rot); if rot==0 => carry_out unchanged, else C=bit31(op2).
//
//   Register (I=0):
//     If bit4==0 => immediate shift by imm5
//     If bit4==1 => register-specified shift by Rs[7:0]
//
//   Shift types:
//     LSL, LSR, ASR, ROR (RRX when imm==0 for ROR immediate only)
// -----------------------------------------------------------------------------
uint32_t dp_operand2(uint32_t instr, uint32_t *carry_out)
{
    const uint32_t I = (instr >> 25) & 1u;

    // -------------------------
    // Immediate (I == 1)  (rotated immediate)
    // -------------------------
    if (I) {
        const uint32_t imm8 =  instr        & 0xFFu;
        const uint32_t rot   = (instr >> 8) & 0xFu;   // rotate right by 2*rot
        if (rot == 0) {
            // No rotation => carry unchanged
            return imm8;
        } else {
            uint32_t val = ror32(imm8, rot * 2u);
            // When the shifter is used (rot>0), C := bit31 of result
            *carry_out = (val >> 31) & 1u;
            return val;
        }
    }

    // -------------------------
    // Register Operand2 (I == 0)
    // -------------------------
    const uint32_t rm    =  instr        & 0xFu;
    const uint32_t rmval = arm_read_src_reg((int)rm);   // PC reads as PC+8
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
                    *carry_out = rmval & 1u;
                    return 0;
                } else { // shimm > 32
                    *carry_out = 0;
                    return 0;
                }

            case 1: // LSR (imm)
                if (shimm == 0) {
                    // LSR #32
                    *carry_out = (rmval >> 31) & 1u;
                    return 0;
                } else if (shimm < 32) {
                    *carry_out = (rmval >> (shimm - 1)) & 1u;
                    return rmval >> shimm;
                } else { // shimm >= 32
                    *carry_out = (shimm == 32) ? ((rmval >> 31) & 1u) : 0;
                    return 0;
                }

            case 2: // ASR (imm)
                if (shimm == 0) {
                    // *** SPECIAL: ASR #0 means ASR #32 ***
                    *carry_out = (rmval >> 31) & 1u;
                    return (rmval & 0x80000000u) ? 0xFFFFFFFFu : 0x00000000u;
                } else if (shimm < 32) {
                    *carry_out = (rmval >> (shimm - 1)) & 1u;
                    return (uint32_t)((int32_t)rmval >> shimm);
                } else { // shimm >= 32 => all sign bits, carry=bit31
                    *carry_out = (rmval >> 31) & 1u;
                    return (rmval & 0x80000000u) ? 0xFFFFFFFFu : 0x00000000u;
                }

            default: // 3: ROR (imm)
                if (shimm == 0) {
                    // RRX: (C << 31) | (rm >> 1); carry_out := bit0 of rm
                    const uint32_t c = (*carry_out) & 1u;
                    *carry_out = rmval & 1u;
                    return (rmval >> 1) | (c << 31);
                } else {
                    // Standard rotate 1..31 (imm5 cannot encode 32 here)
                    *carry_out = (rmval >> (shimm - 1)) & 1u;
                    return ror32(rmval, shimm);
                }
        }
    }

    // -------- register-specified shift (bit4 == 1) --------
    {
        const uint32_t stype = (instr >> 5) & 3u;     // 0 LSL, 1 LSR, 2 ASR, 3 ROR
        const uint32_t rs    = (instr >> 8) & 0xFu;
        // Use low 8 bits of Rs; ARM says amount is Rs[7:0]
        const uint32_t amount = arm_read_src_reg((int)rs) & 0xFFu;

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
                    *carry_out = rmval & 1u;
                    return 0;
                } else {
                    *carry_out = 0;
                    return 0;
                }

            case 1: // LSR (reg)
                if (amount < 32) {
                    *carry_out = (rmval >> (amount - 1)) & 1u;
                    return rmval >> amount;
                } else if (amount == 32) {
                    *carry_out = (rmval >> 31) & 1u;
                    return 0;
                } else {
                    *carry_out = 0;
                    return 0;
                }

            case 2: // ASR (reg)
                if (amount < 32) {
                    *carry_out = (rmval >> (amount - 1)) & 1u;
                    return (uint32_t)((int32_t)rmval >> amount);
                } else {
                    // amount >= 32 => all sign bits; carry=bit31
                    *carry_out = (rmval >> 31) & 1u;
                    return (rmval & 0x80000000u) ? 0xFFFFFFFFu : 0x00000000u;
                }

            default: // 3: ROR (reg)
            {
                const uint32_t rot = amount & 31u;
                if (rot == 0) {
                    // Rotate by a multiple of 32 => value unchanged, C := bit31
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
