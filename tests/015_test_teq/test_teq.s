.syntax unified
    .cpu cortex-a15
    .global _start
    .text

/* TEQ sets N/Z from (Rn ^ Op2). Three cases:
   1) XOR -> 0    => Z=1, N=0
   2) XOR -> nz   => Z=0, N=0
   3) XOR -> msb1 => Z=0, N=1
*/

_start:
    // ---- Case 1: Z=1, N=0 ----
    mov     r0, #0xF0
    mov     r1, #0xF0
    teq     r0, r1
    mov     r2, #1
    .word   0xE1212374        // BKPT

    // ---- Case 2: Z=0, N=0 ----
    mov     r0, #0xAA
    mov     r1, #0x0F
    teq     r0, r1             // 0xA5 (msb=0)
    mov     r2, #2
    .word   0xE1212374         // BKPT

    // ---- Case 3: Z=0, N=1 ----
    mov     r0, #1
    lsl     r0, r0, #31        // r0 = 0x80000000
    mov     r1, #0
    teq     r0, r1
    mov     r2, #3
    .word   0xE1212374         // BKPT
	