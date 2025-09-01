// disasm.c (small starter; grow as you go)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   // snprintf

static inline uint32_t ror32(uint32_t x, unsigned r) {
    r &= 31;
    return (x >> r) | (x << (32 - r));
}

// imm24<<2 with sign extension (for B/BL)
static inline int32_t signext_imm24_shl2(uint32_t imm24) {
    return ((int32_t)(imm24 << 8)) >> 6; // arithmetic >> keeps sign; net <<2
}

static bool disasm_barrier(uint32_t instr, char *out, size_t n) {
    // Matches: DSB/DMB/ISB with SY option (armv7-A encodings)
    // F57FF04F = DSB SY
    // F57FF05F = DMB SY
    // F57FF06F = ISB SY
    if ((instr & 0xFFFFFFCFu) == 0xF57FF04Fu) {
        uint32_t kind = (instr >> 4) & 0x3u;  // 0=DSB, 1=DMB, 2=ISB
        const char *mn = (kind == 0) ? "dsb" : (kind == 1) ? "dmb" : "isb";
        // Option is SY (0xF) in these encodings
        snprintf(out, n, "%s sy", mn);
        return true;
    }
    return false;
}

void disasm_line(uint32_t pc, uint32_t instr, char *out, size_t out_sz) {
    // Default fallback
    snprintf(out, out_sz, ".word 0x%08X", instr);

    // ---- Catch barriers first (preempts LDR/STR mis-decode of F57Fxxxx) ----
    if (disasm_barrier(instr, out, out_sz)) return;

    // ---- MOVW / MOVT (A32) ----
    // MOVW: cond 0011 0000 xxxx xxxx (0x03000000)
    // MOVT: cond 0011 0100 xxxx xxxx (0x03400000)
    if ((instr & 0x0FF00000u) == 0x03000000u || (instr & 0x0FF00000u) == 0x03400000u) {
        uint32_t Rd    = (instr >> 12) & 0xF;
        uint32_t imm4  = (instr >> 16) & 0xF;
        uint32_t imm12 = instr & 0xFFF;
        uint32_t imm16 = (imm4 << 12) | imm12;
        if ((instr & 0x0FF00000u) == 0x03000000u) {
            snprintf(out, out_sz, "movw r%u, #0x%04X", Rd, imm16);
        } else {
            snprintf(out, out_sz, "movt r%u, #0x%04X", Rd, imm16);
        }
        return;
    }

    // ---- B / BL ----
    if ((instr & 0x0E000000u) == 0x0A000000u) {
        uint32_t imm24  = instr & 0x00FFFFFFu;
        int32_t  offset = signext_imm24_shl2(imm24);
        uint32_t target = pc + 8 + (uint32_t)offset;  // ARM pipeline: PC is current+8
        snprintf(out, out_sz, "%s 0x%08X", (instr & 0x01000000u) ? "bl" : "b", target);
        return;
    }

    // ---- Data-processing (immediate) — quick common mnemonics ----
    if ((instr & 0x0C000000u) == 0x00000000u) {
        uint32_t op = (instr >> 21) & 0xF;
        uint32_t S  = (instr >> 20) & 1u;
        uint32_t Rd = (instr >> 12) & 0xF;
        uint32_t Rn = (instr >> 16) & 0xF;

        if (instr & (1u << 25)) { // immediate shifter
            uint32_t imm8  = instr & 0xFF;
            uint32_t rot2  = ((instr >> 8) & 0xF) * 2;
            uint32_t imm32 = ror32(imm8, rot2);

            switch (op) {
                case 0xD: snprintf(out, out_sz, "mov%s r%u, #0x%X", S?"s":"", Rd, imm32); return; // MOV
                case 0xA: snprintf(out, out_sz, "cmp r%u, #0x%X",   Rn, imm32); return;           // CMP
                case 0x8: snprintf(out, out_sz, "tst r%u, #0x%X",   Rn, imm32); return;           // TST
                case 0x2: snprintf(out, out_sz, "sub%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0x4: snprintf(out, out_sz, "add%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0x0: snprintf(out, out_sz, "and%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0x1: snprintf(out, out_sz, "eor%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                case 0xC: snprintf(out, out_sz, "orr%s r%u, r%u, #0x%X", S?"s":"", Rd, Rn, imm32); return;
                // (expand with more ops as you like)
            }
        }
    }

    // ---- Single data transfer (LDR/STR immediate & byte) ----
    if ((instr & 0x0C000000u) == 0x04000000u) {
        uint32_t L = (instr >> 20) & 1u;
        uint32_t B = (instr >> 22) & 1u;
        uint32_t P = (instr >> 24) & 1u;
        uint32_t U = (instr >> 23) & 1u;
        uint32_t Rn= (instr >> 16) & 0xF;
        uint32_t Rd= (instr >> 12) & 0xF;
        uint32_t I = (instr >> 25) & 1u;

        const char *mn = (L ? (B ? "ldrb" : "ldr") : (B ? "strb" : "str"));

        if (!I) { // immediate offset
            uint32_t imm12 = instr & 0xFFF;
            int32_t off = U ? (int32_t)imm12 : -(int32_t)imm12;

            const char *base = (Rn == 13) ? "sp" : (Rn == 15) ? "pc" : NULL;
            char basebuf[6];
            if (!base) { snprintf(basebuf, sizeof basebuf, "r%u", Rn); base = basebuf; }

            if (P) snprintf(out, out_sz, "%s r%u, [%s, #%+d]", mn, Rd, base, off);
            else   snprintf(out, out_sz, "%s r%u, [%s], #%+d", mn, Rd, base, off);
            return;
        }
        // (reg-offset variants can be added later)
    }

    // ---- Block transfers: LDM/STM ----
    if ((instr & 0x0E000000u) == 0x08000000u) {
        uint32_t P = (instr >> 24) & 1u;
        uint32_t U = (instr >> 23) & 1u;
        uint32_t W = (instr >> 21) & 1u;
        uint32_t L = (instr >> 20) & 1u;
        uint32_t Rn = (instr >> 16) & 0xF;
        uint32_t reglist = instr & 0xFFFF;

        const char *mn = L ? "ldm" : "stm";
        const char *am = (!U && !P) ? "da" :
                         (!U &&  P) ? "db" :
                         ( U && !P) ? "ia" : "ib";

        const char *base = (Rn == 13) ? "sp" : (Rn == 15) ? "pc" : NULL;
        char basebuf[8];
        if (!base) { snprintf(basebuf, sizeof basebuf, "r%u", Rn); base = basebuf; }

        char regs[128]; size_t n = 0; bool first = true;
        n += snprintf(regs+n, sizeof regs - n, "{");
        for (int r = 0; r <= 15; ++r) if (reglist & (1u << r)) {
            if (!first) n += snprintf(regs+n, sizeof regs - n, ", ");
            if      (r == 13) n += snprintf(regs+n, sizeof regs - n, "sp");
            else if (r == 14) n += snprintf(regs+n, sizeof regs - n, "lr");
            else if (r == 15) n += snprintf(regs+n, sizeof regs - n, "pc");
            else              n += snprintf(regs+n, sizeof regs - n, "r%d", r);
            first = false;
        }
        n += snprintf(regs+n, sizeof regs - n, "}");
        snprintf(out, out_sz, "%s%s %s%s, %s", mn, am, base, W ? "!" : "", regs);
        return;
    }

    // Fallback already set at the top
}
