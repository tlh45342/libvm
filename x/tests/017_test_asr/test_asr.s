.syntax unified
    .arch armv7-a
    .text
    .global _start

/* Layout of results (r6 base = 0x00100000)
   Off   What
   0x00  ASR#1  (0x80000001) result
   0x04  CPSR
   0x08  ASR#31 (0x80000001) result
   0x0C  CPSR
   0x10  ASR#32 (imm=0) (0x80000001) result
   0x14  CPSR
   0x18  ASR#1  (0x7FFFFFFE) result
   0x1C  CPSR
   0x20  ASR#31 (0x7FFFFFFE) result
   0x24  CPSR
   0x28  ASR#32 (imm=0) (0x7FFFFFFE) result
   0x2C  CPSR
   0x30  ASR r4=0    (0x12345678) result  (carry must be preserved)
   0x34  CPSR
   0x38  ASR r4=8    (0x00008080) result
   0x3C  CPSR
   0x40  ASR r4=40   (0xF0000000) result
   0x44  CPSR
*/
_start:
    /* r6 = 0x00100000 */
    movw    r6, #0
    movt    r6, #0x0010

    /* --- Case A: Rm = 0x80000001 (negative, LSB=1) --- */
    movw    r3, #0x0001
    movt    r3, #0x8000

    /* ASR #1 -> 0xC0000000, C=bit0(original)=1 */
    movs    r5, r3, ASR #1
    mrs     r7, CPSR
    str     r5, [r6, #0x00]
    str     r7, [r6, #0x04]

    /* ASR #31 -> 0xFFFFFFFF, C=bit30(original)=0 */
    movs    r5, r3, ASR #31
    mrs     r7, CPSR
    str     r5, [r6, #0x08]
    str     r7, [r6, #0x0C]

    /* ASR #32 (imm=0 encodes 32) -> 0xFFFFFFFF, C=bit31(original)=1 */
    movs    r5, r3, ASR #0
    mrs     r7, CPSR
    str     r5, [r6, #0x10]
    str     r7, [r6, #0x14]

    /* --- Case B: Rm = 0x7FFFFFFE (positive, LSB=0) --- */
    movw    r3, #0xFFFE
    movt    r3, #0x7FFF

    /* ASR #1 -> 0x3FFFFFFF, C=0 */
    movs    r5, r3, ASR #1
    mrs     r7, CPSR
    str     r5, [r6, #0x18]
    str     r7, [r6, #0x1C]

    /* ASR #31 -> 0x00000000, C=bit30(original)=1 */
    movs    r5, r3, ASR #31
    mrs     r7, CPSR
    str     r5, [r6, #0x20]
    str     r7, [r6, #0x24]

    /* ASR #32 (imm=0) -> 0x00000000, C=bit31(original)=0 */
    movs    r5, r3, ASR #0
    mrs     r7, CPSR
    str     r5, [r6, #0x28]
    str     r7, [r6, #0x2C]

    /* --- Case C: register shifts --- */

    /* Prepare C=1 so we can confirm "shift amount = 0" preserves carry */
    cmp     r0, r0              /* sets Z=1, C=1 */

    /* Rm = 0x12345678, Rn=r4=0  => result unchanged, C preserved (=1) */
    movw    r3, #0x5678
    movt    r3, #0x1234
    mov     r4, #0
    movs    r5, r3, ASR r4
    mrs     r7, CPSR
    str     r5, [r6, #0x30]
    str     r7, [r6, #0x34]

    /* Rm = 0x00008080, r4=8 => 0x00000080, C = bit7(original)=1 */
    movw    r3, #0x8080
    movt    r3, #0x0000
    mov     r4, #8
    movs    r5, r3, ASR r4
    mrs     r7, CPSR
    str     r5, [r6, #0x38]
    str     r7, [r6, #0x3C]

    /* Rm = 0xF0000000 (neg), r4=40 (>=32) => 0xFFFFFFFF, C=bit31(original)=1 */
    movw    r3, #0x0000
    movt    r3, #0xF000
    mov     r4, #40
    movs    r5, r3, ASR r4
    mrs     r7, CPSR
    str     r5, [r6, #0x40]
    str     r7, [r6, #0x44]

    .word   0xDEADBEEF          /* HALT sentinel */
	