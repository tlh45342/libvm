    .global _start
    .text
    .align 2
    .syntax unified

_start:
    // r0 = 5
    mov     r0, #5

    // r1 = r0 - 2  -> expect 3
    sub     r1, r0, #2

    // r2 = r0 - 7  -> expect 0xFFFFFFFE (wraparound)
    sub     r2, r0, #7

    // sentinel halt for the VM
    .word   0xDEADBEEF
