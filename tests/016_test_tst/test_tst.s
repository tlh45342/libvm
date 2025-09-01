        .text
        .global _start
_start:
        MSR     CPSR_f, #0
        MOV     r0, #0xF0
        MOV     r1, #0x0F
        TST     r0, r1               // Z=1
        MRS     r10, CPSR
        AND     r10, r10, #(0xF<<28)

        // Shifter carry path: LSR #1 makes shC = bit0(Rm)
        MOV     r2, #1
        TST     r0, r2, LSR #1       // op2=0, shC=1, Z=1, C=1
        MRS     r11, CPSR
        AND     r11, r11, #(0xF<<28)

        NOP
        .word 0xDEADBEEF
		