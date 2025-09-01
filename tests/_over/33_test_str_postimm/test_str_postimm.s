    .global _start
    .text
    .align 2

_start:
    /* r0 = 0xDEADBEEF */
    movw    r0, #0xBEEF
    movt    r0, #0xDEAD

    /* r2 = 0x00008090 (destination base) */
    movw    r2, #0x8090
    movt    r2, #0x0000

    /* store A, post-increment by 4 */
    str     r0, [r2], #4

    /* r1 = 0xCAFEBABE */
    movw    r1, #0xBABE
    movt    r1, #0xCAFE

    /* store B, post-increment by 4 */
    str     r1, [r2], #4

    /* sentinel halt (your VM stops when it fetches 0xDEADBEEF) */
    .word   0xDEADBEEF
