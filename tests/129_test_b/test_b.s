       .text
        .global _start

_start:
    mov     r0, #1
    cmp     r0, #0          @ Z=0
    beq     done_bad        @ must NOT take
    b       good1

done_bad:
    bkpt    #0x1234

good1:
    mov     r0, #0
    cmp     r0, #0          @ Z=1
    beq     good2           @ must take

   .word   0xDEADBEEF      @ would be wrong if it falls through

good2:
    bkpt    #0x1234
