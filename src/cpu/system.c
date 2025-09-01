// src/cpu/system.c

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"         // CPU struct, extern CPU cpu
#include "cpu_flags.h"   // psr_write(), CPSR_* bits, is_user_mode()
#include "system.h"      // prototypes

// --- Barriers: treat as NOPs in this VM ---
void handle_dsb(uint32_t instr) { (void)instr; }
void handle_dmb(uint32_t instr) { (void)instr; }
void handle_isb(uint32_t instr) { (void)instr; }

// --- Software interrupt / SVC ---
// ARM architectural return address for SVC is the address of the next instr
// in the *original* context. Under the NPC model, that is exactly cpu.npc.
void handle_svc(uint32_t instr) {
    (void)instr; // imm not used

    // Save old CPSR into SPSR_<svc> and LR_<svc> := return address
    cpu.spsr  = cpu.cpsr;
    cpu.r[14] = cpu.npc;                 // preferred return address

    // Enter SVC: mode=0b10011, T=0 (ARM), I=1, clear IT bits
    uint32_t p = cpu.cpsr;
    p = (p & ~0x1Fu) | 0x13u;            // SVC mode
    p &= ~(1u << 5);                      // T=0 (ARM)
    p |=  (1u << 7);                      // I=1 (mask IRQ)
    p &= ~((0x3Fu << 10) | (0x3u << 25)); // clear IT bits
    cpu.cpsr = p;

    // Vector to SVC handler @ 0x00000008 (A profile, low vectors)
    cpu.npc = 0x00000008u;
}

// --- MRS (move PSR to register) ---
// Rd==PC is UNPREDICTABLE; we ignore the write in that case.
void handle_mrs(uint32_t instr) {
    uint32_t Rd  = (instr >> 12) & 0xFu;
    uint32_t sps = (instr >> 22) & 1u;   // 0=CPSR, 1=SPSR
    uint32_t val = sps ? cpu.spsr : cpu.cpsr;

    if (Rd == 15u) return;               // ignore (keeps core robust)
    cpu.r[Rd] = val;
}

// --- MSR helpers ---
static void handle_msr_common(uint32_t instr) {
    int spsr_sel   = (instr >> 22) & 1u;      // 0=CPSR, 1=SPSR
    uint32_t fields= (instr >> 16) & 0xFu;    // {f s x c}
    uint32_t op;

    if ((instr >> 25) & 1u) {
        uint32_t imm8 = instr & 0xFFu;
        uint32_t rot2 = ((instr >> 8) & 0xFu) * 2u;
        op = rot2 ? ((imm8 >> rot2) | (imm8 << (32 - rot2))) : imm8;
    } else {
        uint32_t Rm = instr & 0xFu;
        op = cpu.r[Rm];
    }

    if (spsr_sel) {
        if (!is_user_mode()) {
            psr_write(&cpu.spsr, op, fields, 0);
        }
    } else {
        // psr_write knows how to mask fields per privilege and handle
        // control bits (E, AIF, T, mode) correctly.
        psr_write(&cpu.cpsr, op, fields, 1);
        // If T bit changed here (interworking), fetch/commit glue will
        // observe it on the next instruction via cpu.cpsr.
    }
}

void handle_msr(uint32_t instr)     { handle_msr_common(instr); }
void handle_msr_reg(uint32_t instr) { handle_msr_common(instr); }
void handle_msr_imm(uint32_t instr) { handle_msr_common(instr); }

// --- SETEND (endian select) ---
void handle_setend(uint32_t instr) {
    uint32_t E = (instr >> 9) & 1u;
    if (E) cpu.cpsr |=  CPSR_E; else cpu.cpsr &= ~CPSR_E;
}

// --- CPS (change processor state / mask bits / mode change) ---
void handle_cps(uint32_t instr) {
    bool disable = ((instr >> 18) & 1u) != 0;
    bool Mbit    = ((instr >> 17) & 1u) != 0;

    uint32_t mask = 0;
    if (instr & (1u << 8)) mask |= CPSR_A;
    if (instr & (1u << 7)) mask |= CPSR_I;
    if (instr & (1u << 6)) mask |= CPSR_F;

    if (disable) cpu.cpsr |= mask; else cpu.cpsr &= ~mask;

    if (Mbit) {
        uint32_t mode = instr & 0x1Fu;
        cpu.cpsr = (cpu.cpsr & ~0x1Fu) | (mode & 0x1Fu);
        // (If you later bank SP/LR/SPSR per-mode, do it here.)
    }
}

// -----------------------------------------------------------------------------
// Misc simple handlers referenced by execute()
// -----------------------------------------------------------------------------
void handle_nop(uint32_t instr) { (void)instr; }
void handle_wfi(uint32_t instr) { (void)instr; }   // NOP in this VM

void handle_deadbeef(uint32_t instr) {
    (void)instr;
    cpu.halt_reason = HALT_DEADBEEF;
    cpu_halt();
}

void handle_bkpt(uint32_t instr) {
    (void)instr;
    cpu.halt_reason = HALT_BKPT;
    cpu_halt();
}

// BFC Rd, #lsb, #width   (A32)
// Rd==PC is UNPREDICTABLE: ignore the write.
void handle_bfc(uint32_t instr) {
    uint32_t Rd  = (instr >> 12) & 0xFu;
    uint32_t lsb = (instr >> 7)  & 0x1Fu;
    uint32_t msb = (instr >> 16) & 0x1Fu;

    if (msb < lsb) return;               // UNPREDICTABLE → ignore
    if (Rd == 15u) return;               // UNPREDICTABLE → ignore

    uint32_t raw_width = msb - lsb + 1u;
    uint32_t width     = (raw_width >= 32u) ? 32u : raw_width;

    uint32_t clear_mask = (width == 32u) ? 0u
                          : ~(((1u << width) - 1u) << lsb);

    cpu.r[Rd] &= clear_mask;
    // Flags unaffected
}

// BFI Rd, Rn, #lsb, #width  (A32)
// Rd==PC is UNPREDICTABLE: ignore the write.
void handle_bfi(uint32_t instr) {
    uint32_t Rd  = (instr >> 12) & 0xFu;
    uint32_t Rn  = (instr >> 0)  & 0xFu;
    uint32_t lsb = (instr >> 7)  & 0x1Fu;
    uint32_t msb = (instr >> 16) & 0x1Fu;

    if (msb < lsb) return;               // UNPREDICTABLE → ignore
    if (Rd == 15u) return;               // UNPREDICTABLE → ignore

    uint32_t raw_width = msb - lsb + 1u;
    uint32_t width     = (raw_width >= 32u) ? 32u : raw_width;

    uint32_t field_mask = (width == 32u) ? 0xFFFFFFFFu
                          : ((1u << width) - 1u) << lsb;

    uint32_t src = cpu.r[Rn];
    uint32_t ins = (width == 32u) ? src : ((src << lsb) & field_mask);

    cpu.r[Rd] = (cpu.r[Rd] & ~field_mask) | ins;
    // Flags unaffected.
}

// CLZ Rd, Rm
// Rd==PC is UNPREDICTABLE: ignore the write.
void handle_clz(uint32_t instr) {
    uint32_t Rd = (instr >> 12) & 0xF;
    uint32_t Rm =  instr        & 0xF;
    if (Rd == 15u) return;               // ignore

    uint32_t x  = cpu.r[Rm];
    uint32_t n;
#if defined(__GNUC__) || defined(__clang__)
    n = (x == 0) ? 32u : (uint32_t)__builtin_clz(x);
#else
    if (x == 0) {
        n = 32u;
    } else {
        n = 0u;
        while ((x & 0x80000000u) == 0u) { x <<= 1; n++; }
    }
#endif
    cpu.r[Rd] = n;
    // Flags unaffected.
}
