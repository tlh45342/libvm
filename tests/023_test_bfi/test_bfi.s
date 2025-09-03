    .syntax unified
    .arch armv7-a
    .arm
    .global _start
_start:
    /* r6 = 0x0010_0000 (results base) */
    movw    r6, #0x0000
    movt    r6, #0x0010

    /* --- A: insert low 4 bits at LSB (Rd keeps upper bits) ---
       Rd = 0x12345678, Rn = 0xAAAAAAAA → result 0x1234567A */
    movw    r0, #0x5678
    movt    r0, #0x1234              @ r0 = 0x12345678
    movw    r1, #0xAAAA
    movt    r1, #0xAAAA              @ r1 = 0xAAAAAAAA
    bfi     r0, r1, #0, #4           @ replace bits [3:0] with 0xA
    str     r0, [r6, #0]             @ expect 0x1234567A

    /* --- B: insert one byte at bits 8..15 ---
       Rd = 0xDEADBEEF, Rn = 0x12 → result 0xDEAD12EF */
    movw    r2, #0xBEEF
    movt    r2, #0xDEAD              @ r2 = 0xDEADBEEF
    mov     r3, #0x12                @ r3 = 0x00000012
    bfi     r2, r3, #8, #8
    str     r2, [r6, #4]             @ expect 0xDEAD12EF

    /* --- C: 12-bit field crossing a byte boundary at bit 12 ---
       Rd = 0xFFFF0000, Rn = 0x13579BDF → result 0xFFBDF000 */
    movw    r4, #0x0000
    movt    r4, #0xFFFF              @ r4 = 0xFFFF0000
    movw    r5, #0x9BDF
    movt    r5, #0x1357              @ r5 = 0x13579BDF
    bfi     r4, r5, #12, #12
    str     r4, [r6, #8]             @ expect 0xFFBDF000

    /* --- D: full-width insert (lsb=0, width=32) ---
       Rd = 0xCAFEBABE, Rn = 0x0F0F0F0F → result 0x0F0F0F0F */
    movw    r7, #0xBABE
    movt    r7, #0xCAFE              @ r7 = 0xCAFEBABE
    movw    r8, #0x0F0F
    movt    r8, #0x0F0F              @ r8 = 0x0F0F0F0F
    bfi     r7, r8, #0, #32
    str     r7, [r6, #12]            @ expect 0x0F0F0F0F

    /* --- E: top-nibble insert (bits 31..28) ---
       Rd = 0x01234567, Rn = 0xC → result 0xC1234567 */
    movw    r9,  #0x4567
    movt    r9,  #0x0123             @ r9 = 0x01234567
    mov     r10, #0xC                @ r10 = 0x0000000C
    bfi     r9, r10, #28, #4
    str     r9, [r6, #16]            @ expect 0xC1234567

    /* Halt so the harness can read back */
    bkpt    #0x1234
