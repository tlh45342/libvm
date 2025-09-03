    .syntax unified
    .arch armv7-a
    .text
    .global _start
_start:
    nop

    // Halt
    .word   0xDEADBEEF
