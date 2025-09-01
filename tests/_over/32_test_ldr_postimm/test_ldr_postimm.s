.global _start
    .text
    .align 2

_start:
    ldr     r1, =data        @ uses LDR literal (your VM supports it)
    ldr     r2, [r1], #4     @ LDR post-indexed, word
    ldr     r3, [r1], #4     @ LDR post-indexed, word
    .word   0xDEADBEEF       @ sentinel halt

    .align 2
data:
    .word   0x12345678
    .word   0x9ABCDEF0
