    .arch armv7-a
    .arm
    .text
    .global _start
target:
    add     r0, r0, #7
    bx      lr

_start:
    mov     r0, #1
    ldr     r1, =target
    blx     r1                    @ requires BLX(reg) support
halt:
    .word   0xDEADBEEF
    .ltorg
	