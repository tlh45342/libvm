 .arm
        .text
        .global _start

/* 018_test_lsl — LSL edge-case validation
   r6 base: 0x00100000
   Snapshots:
     [r6+0]  = CPSR after LSLS #1
     [r6+4]  = CPSR after LSLS #0
     [r6+8]  = CPSR after LSLS #31
     [r6+12] = CPSR after LSLS #32 (imm)     EXPECT 0x60000000 (Z=1,C=1)
     [r6+16] = CPSR after LSLS r4=33 (reg)   EXPECT 0x40000000 (Z=1,C=0)
*/

_start:
        /* r6 = 0x00100000 (CPSR snapshot base) */
        movw    r6, #0
        movt    r6, #0x0010

        /* r0 = 1 (so C effects are visible) */
        mov     r0, #1

        /* 1) LSLS #1  (result=2, N=0 Z=0 C=0) */
        /* Encoding: LSLS r2, r0, #1  -> 0xE1B02080 */
        .word   0xE1B02080
        mrs     r3, cpsr
        str     r3, [r6, #0]

        /* 2) LSLS #0  (C unchanged; still 0 here) */
        /* Encoding: LSLS r2, r0, #0  -> 0xE1B02000 */
        .word   0xE1B02000
        mrs     r3, cpsr
        str     r3, [r6, #4]

        /* 3) LSLS #31 (result=0x80000000, N=1 Z=0 C=0) */
        /* Encoding: LSLS r2, r0, #31 -> 0xE1B02F80 */
        .word   0xE1B02F80
        mrs     r3, cpsr
        str     r3, [r6, #8]

        /* 4) LSLS #32 (imm) (result=0, Z=1, C=bit0(r0)=1) */
        /* Encoding: LSLS r2, r0, #32 -> 0xE1B02410 */
        .word   0xE1B02410
        mrs     r3, cpsr
        str     r3, [r6, #12]

        /* 5) LSLS by register: r4 = 33 (>32) (result=0, Z=1, C=0) */
        mov     r4, #33
        /* Encoding: LSLS r2, r0, r4 -> 0xE1B02014  (REGISTER-SPECIFIED SHIFT) */
        .word   0xE1B02014
        mrs     r3, cpsr
        str     r3, [r6, #16]

        /* Echo snapshots into regs for easy viewing */
        ldr     r7, [r6, #12]      /* expect 0x60000000 after imm #32 */
        ldr     r8, [r6, #16]      /* expect 0x40000000 after reg 33  */

        /* Stop */
        bkpt    0
		