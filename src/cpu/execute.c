// src/cpu/execute.c  — fast key12 dispatcher + xmask32 disambiguation
// O(1) bucket -> per-bucket sorted candidates (by specificity) -> cond check -> handler

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>

#include "cpu.h"      // extern CPU cpu; extern debug_flags_t debug_flags;
#include "log.h"      // log_printf
#include "cond.h"     // evaluate_condition()
#include "shifter.h"  // helpers if handlers need them

// Handlers (existing headers in your tree)
#include "alu.h"
#include "branch.h"
#include "system.h"
#include "logic.h"
#include "memops.h"
#include "execute.h"
#include "arm_mul.h"

// ------------------------ key12 helper ------------------------
static inline uint16_t key12(uint32_t instr) {
    uint32_t op1 = (instr >> 25) & 0x7;   // bits 27:25
    uint32_t op2 = (instr >> 20) & 0x1F;  // bits 24:20
    uint32_t op3 = (instr >>  4) & 0x0F;  // bits  7:4
    return (uint16_t)((op1 << 9) | (op2 << 4) | op3);
}

typedef void (*insn_handler_t)(uint32_t instr);

typedef struct {
    uint16_t       mask12;      // mask in 12-bit key space
    uint16_t       value12;     // value in 12-bit key space
    uint32_t       xmask32;     // OPTIONAL exact disambiguation (0 => none)
    uint32_t       xvalue32;    // value for xmask32
    bool           check_cond;  // apply cond? (false for DEADBEEF/NOP/WFI/DSB/DMB/ISB)
    insn_handler_t fn;          // handler
    const char*    name;        // for logs
} k12_entry;

// -------------------- seed rules (order not critical; priority is) --------------------
static const k12_entry K12_TABLE[] = {
	// --- Bitfield Clear (BFC), A32 --- (already added, shown for context)
	{ 0x0FFFu, 0x07D1u, 0x0FE3C1FFu, 0x07C3001Fu, true, handle_bfc, "BFC" },
	{ 0x0FFFu, 0x07C1u, 0x0FE3C1FFu, 0x07C3001Fu, true, handle_bfc, "BFC" },

	// --- Bitfield Insert (BFI), A32 ---
	{ 0x0FFFu, 0x07C1u, 0x0FE00070u, 0x07C00010u, true, handle_bfi, "BFI" },
	{ 0x0FFFu, 0x07D1u, 0x0FE00070u, 0x07C00010u, true, handle_bfi, "BFI" },

	// BEFORE CMN
	{ 0x0FFFu, 0x0161u, 0x0FFF0FF0u, 0x016F0F10u, true, handle_clz, "CLZ" },

	// Key12 for these words shows up as 0x574 in your logs; we use a tight 32-bit mask
	{ 0x0FFFu, 0x0574u, 0x0FFF0FF0u, 0x057F0040u, false, handle_dsb, "DSB" }, // F57FF04x
	{ 0x0FFFu, 0x0575u, 0x0FFF0FF0u, 0x057F0050u, false, handle_dmb, "DMB" }, // F57FF05x
	{ 0x0FFFu, 0x0576u, 0x0FFF0FF0u, 0x057F0060u, false, handle_isb, "ISB" }, // F57FF06x

	// LDR (literal): I=0,P=1,W=0,L=1 and Rn=PC (r15)
	// xmask  = (I|P|W|L|Rn) = 0x02000000|0x01000000|0x00200000|0x00100000|0x000F0000 = 0x033F0000
	// xvalue = (   0|  1   |   0   |  1   |   0xF   ) = 0x011F0000
	{ 0x0F50u, 0x0510u, 0x033F0000u, 0x011F0000u, true, handle_ldr_literal, "LDR(literal)" },

	// BKPT (ARM state): cond fixed to AL, opcode pattern 0xE1200070 with 12-bit imm
	{ 0x0FFFu, 0x0127u, 0x0FF000F0u, 0x01200070u, false, handle_bkpt, "BKPT" },

	{ 0x0F00u, 0x0100u, 0xFF00F000u, 0xF1000000u, true, handle_cps, "CPS" },

    // ---- Wide moves (exact patterns) ----
    { 0x0F00u, 0x0300u, 0x0FF00000u, 0x03000000u, true, handle_movw, "MOVW" },
    { 0x0F00u, 0x0300u, 0x0FF00000u, 0x03400000u, true, handle_movt, "MOVT" },

    // ---- Doubleword transfers (final: correct for your toolchain) ----
    // key12 gate: op1==0 (bits 27..25) AND op3==0xD/0xF (bits 7..4)
    // xmask32: split imm vs reg by bit22 (0x0040_0000)
    { 0x0E0Fu, 0x000Du, 0x00400000u, 0x00400000u, true, exec_ldrd_imm, "LDRD(imm)" }, // op3=D?, imm (bit22=1)
    { 0x0E0Fu, 0x000Du, 0x00400000u, 0x00000000u, true, exec_ldrd_reg, "LDRD(reg)" }, // op3=D?, reg (bit22=0)
    { 0x0E0Fu, 0x000Fu, 0x00400000u, 0x00400000u, true, exec_strd_imm, "STRD(imm)" }, // op3=F?, imm (bit22=1)
    { 0x0E0Fu, 0x000Fu, 0x00400000u, 0x00000000u, true, exec_strd_reg, "STRD(reg)" }, // op3=F?, reg (bit22=0)

    // ---- DP (register) class (I,S ignored in mask) ----
    { 0x0DE0u, 0x0000u, 0, 0, true, handle_and, "AND" },
    { 0x0DE0u, 0x0020u, 0, 0, true, handle_eor, "EOR" },
    { 0x0DE0u, 0x0040u, 0, 0, true, handle_sub, "SUB" },
    { 0x0DE0u, 0x0060u, 0, 0, true, handle_rsb, "RSB" },
    { 0x0DE0u, 0x0080u, 0, 0, true, handle_add, "ADD" },
    { 0x0DE0u, 0x00A0u, 0, 0, true, handle_adc, "ADC" },
    { 0x0DE0u, 0x00C0u, 0, 0, true, handle_sbc, "SBC" },
    { 0x0DE0u, 0x00E0u, 0, 0, true, handle_rsc, "RSC" },
    { 0x0DE0u, 0x0100u, 0, 0, true, handle_tst, "TST" },
    { 0x0DE0u, 0x0120u, 0, 0, true, handle_teq, "TEQ" },
    { 0x0DE0u, 0x0140u, 0, 0, true, handle_cmp, "CMP" },
    { 0x0DE0u, 0x0160u, 0, 0, true, handle_cmn, "CMN" },
    { 0x0DE0u, 0x0180u, 0, 0, true, handle_orr, "ORR" },
    { 0x0DE0u, 0x01A0u, 0, 0, true, handle_mov_dp, "MOV" }, // DP MOV
    { 0x0DE0u, 0x01C0u, 0, 0, true, handle_bic, "BIC" },
    { 0x0DE0u, 0x01E0u, 0, 0, true, handle_mvn, "MVN" },

    // ---- DP (immediate) class (I=1) ----
    { 0x0FE0u, 0x0200u, 0, 0, true, handle_and,     "AND (imm)" },
    { 0x0FE0u, 0x0310u, 0, 0, true, handle_tst_imm, "TST (imm)" },
    { 0x0FE0u, 0x0350u, 0, 0, true, handle_cmp_imm, "CMP (imm)" }, // 0x35x
    { 0x0FE0u, 0x0330u, 0, 0, true, handle_teq,     "TEQ (imm)" },
    { 0x0FE0u, 0x0370u, 0, 0, true, handle_cmn,     "CMN (imm)" },
    { 0x0FE0u, 0x0380u, 0, 0, true, handle_orr,     "ORR (imm)" },
    { 0x0FE0u, 0x03A0u, 0, 0, true, handle_mov_imm, "MOV (imm)" },
    { 0x0FE0u, 0x03C0u, 0, 0, true, handle_bic,     "BIC (imm)" },
    { 0x0FE0u, 0x03E0u, 0, 0, true, handle_mvn,     "MVN (imm)" },

    // ---- Single data transfer ----
	{ 0x0F50u, 0x0510u, 0, 0, true, handle_ldr_preimm,   "LDR  pre-imm" },
    { 0x0F50u, 0x0500u, 0, 0, true, handle_str_preimm,   "STR  pre-imm" },
    { 0x0F50u, 0x0540u, 0, 0, true, handle_strb_preimm,  "STRB pre-imm" },
    { 0x0F50u, 0x0550u, 0, 0, true, handle_ldrb_preimm,  "LDRB pre-imm" },
    { 0x0F50u, 0x0440u, 0, 0, true, handle_strb_postimm, "STRB(post,imm)" },
    { 0x0F50u, 0x0410u, 0, 0, true, handle_ldr_postimm,  "LDR (post,imm)" },
    { 0x0F70u, 0x0400u, 0, 0, true, handle_str_postimm,  "STR (post,imm)" },
    { 0x0F70u, 0x0450u, 0, 0, true, handle_ldrb_postimm, "LDRB post-imm"  }, // W=0
    { 0x0F70u, 0x0470u, 0, 0, true, handle_ldrb_postimm, "LDRB post-imm W"}, // W=1
    { 0x0E50u, 0x0610u, 0, 0, true, handle_ldr_regoffset,"LDR reg-offset" },

	// Multiply-Long: cond | 00001 | U | A | S | RdHi | RdLo | Rs | 1001 | Rm
	// xmask fixes 27..23 and 7..4, and U/A; S is don't-care.
	{ 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00800090u, true, handle_umull, "UMULL" }, // U=0 A=0
	{ 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00A00090u, true, handle_umlal, "UMLAL" }, // U=0 A=1
	{ 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00C00090u, true, handle_smull, "SMULL" }, // U=1 A=0
	{ 0x0E0Fu, 0x0009u, 0x0FE000F0u, 0x00E00090u, true, handle_smlal, "SMLAL" }, // U=1 A=1

    // ---- LDRB (register) split ----
    { 0x0F7Fu, 0x0750u, 0, 0, true, handle_ldrb_reg,       "LDRB reg pre LSL#0" },
    { 0x0F71u, 0x0750u, 0, 0, true, handle_ldrb_reg_shift, "LDRB reg imm-shift" },

    // ---- Stack-ish specials ----
    { 0x0FF0u, 0x0520u, 0x000F0000u, 0x000D0000u, true, handle_str_predec, "STR(pre-dec SP,imm)" },
    { 0x0FFFu, 0x0490u, 0x0FFFF000u, 0x049DF000u, true, handle_pop_pc,     "POP{..,pc}" },

    // ---- Branch family ----
    { 0x0FFFu, 0x0121u, 0, 0, true, handle_bx, "BX" },
    { 0x0F00u, 0x0A00u, 0, 0, true, handle_b,  "B"  },
    { 0x0F00u, 0x0B00u, 0, 0, true, handle_bl, "BL" },

    // ---- System / exact 32-bit patterns ----
    { 0x0F00u, 0x0F00u, 0x0F000000u, 0x0F000000u, true,  handle_svc, "SVC" },
    { 0x0FBFu, 0x0100u, 0x0FBF0FFFu, 0x010F0000u, true,  handle_mrs, "MRS" },
    { 0x0FFFu, 0x0320u, 0xFFFFFFFFu, 0xE320F000u, false, handle_nop, "NOP" },
    { 0x0FFFu, 0x0320u, 0xFFFFFFFFu, 0xE320F003u, false, handle_wfi, "WFI" },

    // ---- Easter egg / halt ----
    { 0x0FFFu, 0x0EAEu, 0xFFFFFFFFu, 0xDEADBEEFu, false, handle_deadbeef, "DEADBEEF" },
};


// -------------------- 4096 per-key candidate lists --------------------
#define KEY12_SPACE 4096
#define MAX_PER_KEY 16

static uint16_t g_keylist[KEY12_SPACE][MAX_PER_KEY];
static uint8_t  g_keyprio[KEY12_SPACE][MAX_PER_KEY];
static uint8_t  g_keycount[KEY12_SPACE];
static bool     g_k12_ready = false;

// --- popcounts for priority ---
static inline uint8_t popcnt16(uint16_t x){
    x = (x & 0x5555u) + ((x >> 1) & 0x5555u);
    x = (x & 0x3333u) + ((x >> 2) & 0x3333u);
    x = (x + (x >> 4)) & 0x0F0Fu;
    x = x + (x >> 8);
    return (uint8_t)(x & 0x1Fu);
}
static inline uint8_t popcnt32(uint32_t x){
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return (uint8_t)(x & 0x3Fu);
}

// Priority = specificity of mask12 + specificity of xmask32.
// This makes MOVW/MOVT outrank generic DP; STRD/LDRD (tiny xmask32) won’t outrank them.
static inline uint8_t k12_priority(const k12_entry *e) {
    uint8_t p = popcnt16(e->mask12);   // 0..12
    p += popcnt32(e->xmask32);         // 0..32 (often small; MOVW/MOVT are larger)
    return p;
}

// Build per-key lists, sorted by priority (desc). Seed order is irrelevant.
static void k12_build_table(void) {
    for (int k = 0; k < KEY12_SPACE; ++k) g_keycount[k] = 0;

    const size_t N = sizeof(K12_TABLE)/sizeof(K12_TABLE[0]);
    for (int k = 0; k < KEY12_SPACE; ++k) {
        uint8_t cnt = 0;
        for (uint16_t ei = 0; ei < N; ++ei) {
            const k12_entry *e = &K12_TABLE[ei];
            if (((uint16_t)k & e->mask12) == e->value12) {
                uint8_t pr = k12_priority(e);

                if (cnt < MAX_PER_KEY) {
                    // insert by priority (desc), stable on ties
                    uint8_t pos = cnt;
                    while (pos > 0 && pr > g_keyprio[k][pos-1]) {
                        g_keyprio[k][pos] = g_keyprio[k][pos-1];
                        g_keylist[k][pos] = g_keylist[k][pos-1];
                        --pos;
                    }
                    g_keyprio[k][pos] = pr;
                    g_keylist[k][pos] = ei;
                    ++cnt;
                } else {
                    // list full: bump in only if more specific than last slot
                    if (pr > g_keyprio[k][cnt-1]) {
                        uint8_t pos = cnt - 1;
                        while (pos > 0 && pr > g_keyprio[k][pos-1]) {
                            g_keyprio[k][pos] = g_keyprio[k][pos-1];
                            g_keylist[k][pos] = g_keylist[k][pos-1];
                            --pos;
                        }
                        g_keyprio[k][pos] = pr;
                        g_keylist[k][pos] = ei;
                    }
                }
            }
        }
        g_keycount[k] = cnt;
    }
    g_k12_ready = true;
}

static inline void k12_ensure_built(void) {
    if (!g_k12_ready) k12_build_table();
}

// ---------------------- fast dispatcher with xmask32 ----------------------
static inline bool try_decode_key12_fast(uint32_t instr) {
    uint16_t k = key12(instr);

    if (debug_flags & DBG_K12) {
        log_printf("[K12] key=0x%03X op1=%u op2=%u op3=%u\n",
                   k, (instr>>25)&7, (instr>>20)&31, (instr>>4)&15);
    }

    uint8_t cnt = g_keycount[k];
    for (uint8_t i = 0; i < cnt; ++i) {
        const k12_entry *e = &K12_TABLE[g_keylist[k][i]];

        // Optional exact 32-bit disambiguation
        if (e->xmask32 && ((instr & e->xmask32) != e->xvalue32))
            continue;

        // Condition check (unless suppressed for this entry)
        if (e->check_cond) {
            uint8_t cond = (instr >> 28) & 0xF;
            if (cond != 0xF && !evaluate_condition(cond)) {
                if (debug_flags & DBG_K12)
                    log_printf("[K12] %s cond fail (0x%X)\n", e->name, cond);
                return true; // decoded but skipped by condition
            }
        }

        if (debug_flags & DBG_K12)
            log_printf("[K12] %s match (key=0x%03X)\n", e->name, k);

        e->fn(instr);
        return true;
    }
    return false; // no rule matched this key/xmask
}

// ------------------------------- executor --------------------------------
bool execute(uint32_t instr) {
    k12_ensure_built();

    if (debug_flags & DBG_TRACE) {
        uint32_t pc = cpu.r[15];
        log_printf("[TRACE] PC=0x%08X Instr=0x%08X\n", pc, instr);
    }

    if (try_decode_key12_fast(instr))
        return true;

    // Unknown
    uint32_t pc = cpu.r[15];
    log_printf("[ERROR] Unknown instruction: 0x%08X at PC=0x%08X\n", instr, pc);
    uint32_t op    = (instr >> 24) & 0xF;
    uint32_t subop = (instr >> 20) & 0xF;
    log_printf("  [Not matched] op=0x%X subop=0x%X\n", op, subop);
    cpu_halt();
    return false;
}
