// src/cpu/decode_table.c

static const k12_entry g_entries[] = {
    // Put the most specific / exact patterns FIRST
    // System/exact
    { 0x0FFFu, 0x0320u, 0xFFFFFFFFu, 0xE320F000u, false, handle_nop,  "NOP" },
    { 0x0FFFu, 0x0320u, 0xFFFFFFFFu, 0xE320F003u, false, handle_wfi,  "WFI" },
    { 0x0FFFu, 0x0574u, 0xFFFFFFFFu, 0xF57FF04Fu, false, handle_dsb,  "DSB" },
    { 0x0FFFu, 0x0575u, 0xFFFFFFFFu, 0xF57FF05Fu, false, handle_dmb,  "DMB" },
    { 0x0FFFu, 0x0576u, 0xFFFFFFFFu, 0xF57FF06Fu, false, handle_isb,  "ISB" },
    { 0x0FBFu, 0x0100u, 0x0FBF0FFFu, 0x010F0000u, true,  handle_mrs,  "MRS" },
    { 0x0FFFu, 0x0EAEu, 0xFFFFFFFFu, 0xDEADBEEFu, false, handle_deadbeef, "DEADBEEF" },

    // Branch family
    { 0x0FFFu, 0x0121u, 0, 0, true, handle_bx,   "BX" },
    { 0x0F00u, 0x0B00u, 0, 0, true, handle_bl,   "BL" },
    { 0x0F00u, 0x0A00u, 0, 0, true, handle_b,    "B"  },

    // Wide moves – make sure these are before generic DP
    { 0x0F00u, 0x0300u, 0x0FF00000u, 0x03000000u, true, handle_movw, "MOVW" },
    { 0x0F00u, 0x0340u, 0x0FF00000u, 0x03400000u, true, handle_movt, "MOVT" },

    // Single data transfer common forms (pre/post, byte, etc.)
    { 0x0F50u, 0x0510u, 0, 0, true, handle_ldr_preimm,   "LDR  pre-imm" },
    { 0x0F50u, 0x0500u, 0, 0, true, handle_str_preimm,   "STR  pre-imm" },
    { 0x0F50u, 0x0550u, 0, 0, true, handle_ldrb_preimm,  "LDRB pre-imm" },
    { 0x0F70u, 0x0450u, 0, 0, true, handle_ldrb_postimm, "LDRB post-imm" },
    { 0x0E50u, 0x0610u, 0, 0, true, handle_ldr_regoffset,"LDR reg-offset" },

    // DP (register/imm) generic families (mask ignores I and S accordingly)
    { 0x0DE0u, 0x0000u, 0, 0, true, handle_and, "AND" },
    { 0x0DE0u, 0x0180u, 0, 0, true, handle_orr, "ORR" },
    { 0x0DE0u, 0x01A0u, 0, 0, true, handle_mov, "MOV" },
    { 0x0DE0u, 0x01E0u, 0, 0, true, handle_mvn, "MVN" },
    { 0x0DE0u, 0x0100u, 0, 0, true, handle_tst, "TST" },
    { 0x0DE0u, 0x0140u, 0, 0, true, handle_cmp, "CMP" },

    // DP immediate variants (if you keep separate quick-paths)
    { 0x0FE0u, 0x0380u, 0, 0, true, handle_orr,     "ORR (imm)" },
    { 0x0FE0u, 0x03A0u, 0, 0, true, handle_mov_imm, "MOV (imm)" },
    { 0x0FE0u, 0x03E0u, 0, 0, true, handle_mvn,     "MVN (imm)" },
    { 0x0FE0u, 0x0310u, 0, 0, true, handle_tst_imm, "TST (imm)" },
    { 0x0FE0u, 0x0350u, 0, 0, true, handle_cmp_imm, "CMP (imm)" },

    // Block transfers
    { 0x0E10u, 0x0800u, 0, 0, true, handle_stm, "STM" },
    { 0x0E10u, 0x0810u, 0, 0, true, handle_ldm, "LDM" },
};