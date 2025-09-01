        .syntax unified
        .arm
        .text
        .global _start
_start:
        // r0 = 0xF0F0F0F0
        movw    r0, #0x00F0
        movt    r0, #0xF0F0

        // r1 = 0x0F0F0F0F
        movw    r1, #0x0F0F
        movt    r1, #0x0F0F

        // --- original three cases ---

        // [AND reg] r2 = r0 & r1 -> 0
        and     r2, r0, r1

        // [ANDS regs] r3 = r0 & r0 -> r0, sets flags (N from r0)
        ands    r3, r0, r0

        // [AND imm] encodable imm: 0xFF000000 (imm8=0xFF, rot=12)
        and     r4, r0, #0xFF000000

        // --- reg-shifted-by-reg (LSR r2) with S=1 ---
        // r1 = 0x80000001; r2 = 1 -> (r1 LSR r2) = 0x40000000, C=old bit0=1
        movw    r1, #0x0001
        movt    r1, #0x8000
        mov     r2, #1
        ands    r5, r1, r1, lsr r2     // r5 = 0x40000000

        // --- ANDS immediate with encodable rotate result (0x40000000) ---
        // gas will encode this as imm8+rotate
        ands    r6, r0, #0x40000000    // r6 = 0x40000000

        // --- LSL #0 preserves C; we won’t force a specific C here, just exercise the path ---
        ands    r7, r0, r0, lsl #0     // r7 = r0

        // --- ROR #0 means RRX (uses old C as top bit, C=old bit0 of Rm) ---
        ands    r8, r1, r1, ror #0     // r8 = (C<<31) | (r1>>1)

        // --- Rd == Rn aliasing: read Rn before writeback ---
        and     r0, r0, r1

        // --- cond-fail: set Z=1 so ANDNE is skipped ---
        mov     r9, #0
        tst     r9, r9                  // Z=1
        andne   r10, r0, r0             // skipped

        // Halt
        .word   0xDEADBEEF
