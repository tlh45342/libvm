.global _start
.section .text

_start:
    BL target     // Should jump to target, then return
    .word 0xdeadbeef

target:
    MOV r0, #0x42
    BX LR
	
