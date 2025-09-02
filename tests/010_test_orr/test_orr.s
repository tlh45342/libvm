    .arch armv7-a
    .arm
    .text
    .global _start

/* Test ORR instruction forms */

_start:
    /* --- ORR (immediate) --- */
    mov     r0, #0xF0          @ r0 = 0x000000F0
    orr     r1, r0, #0x0F      @ r1 = 0x000000FF

    /* --- ORR (register) --- */
    mov     r2, #0x0F          @ r2 = 0x0000000F
    orr     r3, r1, r2         @ r3 = 0x000000FF (unchanged, just form coverage)

    /* --- ORRS (register, shifted) --- */
    mov     r4, #0             @ clear r4
    orr     r4, r4, #1         @ r4 = 0x00000001
    mov     r5, #1
    orrs    r6, r4, r5, lsl #1 @ r6 = 0x00000003, flags updated (N=0, Z=0)
    mrs     r10, cpsr          @ snapshot flags from ORRS

    /* --- Conditional ORR/MOV (EQ should NOT execute because Z=0) --- */
    orreq   r7, r7, #0xFF      @ skipped; r7 stays 0
    moveq   r7, #1             @ skipped; r7 stays 0

halt:
    bkpt    #0x1234
	