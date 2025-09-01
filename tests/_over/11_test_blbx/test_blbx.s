  .text
    .global _start

_start:
    ldr     sp, =stack_top     @ Set stack pointer to 8MB above code
    mov     r0, #0x42          @ Load test value
    bl      subroutine         @ Call subroutine
    b       halt               @ Infinite halt

subroutine:
    mov     r1, r0             @ Copy r0 to r1
    bx      lr                 @ Return from subroutine

halt:
    .word   0xDEADBEEF         @ Sentinel value for emulator

    .align 4
    .ltorg

stack_top = . + 0x800000       @ Set stack 8MB beyond end of code
