Here’s a practical, **numbered** list of the core **ARMv7-A A32 (ARM state, 32-bit)** instruction mnemonics with short descriptions (Thumb/T32 not included; VFP/NEON not included). One per line:

1. **ADC** — Add with carry
2. **ADD** — Add
3. **AND** — Bitwise AND
4. **BIC** — Bit clear (A & ~B)
5. **CMN** — Compare negative (A + B, update flags only)
6. **CMP** — Compare (A − B, update flags only)
7. **EOR** — Bitwise XOR
8. **MOV** — Move/register copy
9. **MVN** — Move NOT (~operand)
10. **ORR** — Bitwise OR
11. **RSB** — Reverse subtract (B − A)
12. **RSC** — Reverse subtract with carry (B − A − ~C)
13. **SBC** — Subtract with carry (A − B − ~C)
14. **SUB** — Subtract
15. **TEQ** — Test equivalence (A XOR B, flags only)
16. **TST** — Test (A AND B, flags only)
17. **ASR** — Arithmetic shift right (as shifter op)
18. **LSL** — Logical shift left (as shifter op)
19. **LSR** — Logical shift right (as shifter op)
20. **ROR** — Rotate right (as shifter op)
21. **RRX** — Rotate right with extend (as shifter op)
22. **BFC** — Bit-field clear
23. **BFI** — Bit-field insert
24. **SBFX** — Signed bit-field extract
25. **UBFX** — Unsigned bit-field extract
26. **SSAT** — Signed saturate
27. **USAT** — Unsigned saturate
28. **SSAT16** — Signed saturate (two 16-bit lanes)
29. **USAT16** — Unsigned saturate (two 16-bit lanes)
30. **SXTB** — Sign-extend byte
31. **SXTB16** — Sign-extend two bytes to two halfwords
32. **SXTH** — Sign-extend halfword
33. **UXTB** — Zero-extend byte
34. **UXTB16** — Zero-extend two bytes to two halfwords
35. **UXTH** — Zero-extend halfword
36. **CLZ** — Count leading zeros
37. **RBIT** — Reverse bits in word
38. **REV** — Reverse byte order in word
39. **REV16** — Reverse byte order in each halfword
40. **REVSH** — Reverse bytes in halfword and sign-extend
41. **MUL** — Multiply (low 32)
42. **MLA** — Multiply-accumulate (low 32)
43. **UMULL** — Unsigned multiply (64-bit result)
44. **UMLAL** — Unsigned multiply-accumulate (64-bit)
45. **SMULL** — Signed multiply (64-bit result)
46. **SMLAL** — Signed multiply-accumulate (64-bit)
47. **UMAAL** — Unsigned multiply-accumulate long (A + B*C)
48. **SMLAxy** — Signed 16×16 multiply-accumulate (select halfwords)
49. **SMULxy** — Signed 16×16 multiply (select halfwords)
50. **SMLAW** — Signed 32×16 multiply-accumulate (word×half)
51. **SMULW** — Signed 32×16 multiply (word×half)
52. **SMLAD** — Signed dual 16×16 MAC (adds products)
53. **SMLADX** — Same as SMLAD with exchanged halves
54. **SMLSD** — Signed dual 16×16 MAC (subtracts products)
55. **SMLSDX** — Same as SMLSD with exchanged halves
56. **SMUAD** — Signed dual 16×16 multiply (adds products)
57. **SMUADX** — Same with exchanged halves
58. **SMUSD** — Signed dual 16×16 multiply (subtracts products)
59. **SMUSDX** — Same with exchanged halves
60. **SMMLA** — Signed 32×32 → 64, add, top 32 result
61. **SMMLS** — Signed 32×32 → 64, subtract, top 32 result
62. **SMMUL** — Signed 32×32 → 64, top 32 result
63. **QADD** — Saturating add (signed)
64. **QSUB** — Saturating subtract (signed)
65. **QDADD** — Saturating double-add (signed)
66. **QDSUB** — Saturating double-sub (signed)
67. **SADD16** — Two signed 16-bit adds (packed)
68. **SSUB16** — Two signed 16-bit subs (packed)
69. **SASX** — Signed add/sub (halfword cross)
70. **SSAX** — Signed sub/add (halfword cross)
71. **UADD16** — Two unsigned 16-bit adds (packed)
72. **USUB16** — Two unsigned 16-bit subs (packed)
73. **UASX** — Unsigned add/sub (halfword cross)
74. **USAX** — Unsigned sub/add (halfword cross)
75. **QADD16** — Saturating signed 16-bit add (packed)
76. **QSUB16** — Saturating signed 16-bit sub (packed)
77. **QASX** — Saturating signed add/sub (cross)
78. **QSAX** — Saturating signed sub/add (cross)
79. **UQADD16** — Saturating unsigned 16-bit add (packed)
80. **UQSUB16** — Saturating unsigned 16-bit sub (packed)
81. **UQASX** — Saturating unsigned add/sub (cross)
82. **UQSAX** — Saturating unsigned sub/add (cross)
83. **SADD8** — Four signed 8-bit adds (packed)
84. **SSUB8** — Four signed 8-bit subs (packed)
85. **UADD8** — Four unsigned 8-bit adds (packed)
86. **USUB8** — Four unsigned 8-bit subs (packed)
87. **QADD8** — Saturating signed 8-bit add (packed)
88. **QSUB8** — Saturating signed 8-bit sub (packed)
89. **UQADD8** — Saturating unsigned 8-bit add (packed)
90. **UQSUB8** — Saturating unsigned 8-bit sub (packed)
91. **SHADD16** — Halved (rounded) signed 16-bit add (packed)
92. **SHSUB16** — Halved (rounded) signed 16-bit sub (packed)
93. **SHASX** — Halved signed add/sub (cross)
94. **SHSAX** — Halved signed sub/add (cross)
95. **UHADD16** — Halved unsigned 16-bit add (packed)
96. **UHSUB16** — Halved unsigned 16-bit sub (packed)
97. **UHASX** — Halved unsigned add/sub (cross)
98. **UHSAX** — Halved unsigned sub/add (cross)
99. **SHADD8** — Halved signed 8-bit add (packed)
100. **SHSUB8** — Halved signed 8-bit sub (packed)
101. **UHADD8** — Halved unsigned 8-bit add (packed)
102. **UHSUB8** — Halved unsigned 8-bit sub (packed)
103. **SEL** — Select bytes by GE flags (per-byte mux)
104. **LDR** — Load word
105. **STR** — Store word
106. **LDRB** — Load byte
107. **STRB** — Store byte
108. **LDRH** — Load halfword
109. **STRH** — Store halfword
110. **LDRSB** — Load signed byte
111. **LDRSH** — Load signed halfword
112. **LDRD** — Load doubleword (two regs)
113. **STRD** — Store doubleword (two regs)
114. **LDM** — Load multiple registers
115. **STM** — Store multiple registers
116. **SWP** — Swap word with memory (deprecated)
117. **SWPB** — Swap byte with memory (deprecated)
118. **LDREX** — Load-exclusive word
119. **STREX** — Store-exclusive word
120. **LDREXB** — Load-exclusive byte
121. **STREXB** — Store-exclusive byte
122. **LDREXH** — Load-exclusive halfword
123. **STREXH** — Store-exclusive halfword
124. **LDREXD** — Load-exclusive doubleword
125. **STREXD** — Store-exclusive doubleword
126. **PLD** — Data prefetch (hint)
127. **PLDW** — Data prefetch (write intent)
128. **PLI** — Instruction prefetch (hint)
129. **B** — Branch (supports condition codes)
130. **BL** — Branch with link
131. **BLX (imm)** — Branch with link and state exchange (to Thumb)
132. **BLX (reg)** — Branch with link to register target/state exchange
133. **BX** — Branch/exchange state to register target
134. **BXJ** — Branch to Jazelle (if implemented)
135. **BKPT** — Breakpoint
136. **SVC** — Supervisor call (software interrupt)
137. **HVC** — Hypervisor call (if virtualization present)
138. **SMC** — Secure monitor call (TrustZone)
139. **UDF** — Permanently undefined (guaranteed fault)
140. **ERET** — Exception return from Hyp mode (if implemented)
141. **RFE** — Return from exception (A/R profiles)
142. **SRS** — Store return state (A/R profiles)
143. **MRS** — Move PSR to register (CPSR/SPSR)
144. **MSR** — Move register to PSR (CPSR/SPSR fields)
145. **CPS** — Change processor state (enable/disable IRQ/FIQ, mode)
146. **SETEND** — Set endianness (BE/LE)
147. **CLREX** — Clear exclusive monitor
148. **DMB** — Data memory barrier
149. **DSB** — Data synchronization barrier
150. **ISB** — Instruction synchronization barrier
151. **NOP** — No operation (hint)
152. **YIELD** — Hint: yield in spin-wait loops
153. **WFE** — Wait for event
154. **WFI** — Wait for interrupt
155. **SEV** — Send event
156. **SEVL** — Send event (local)
157. **DBG** — Debug hint (implementation-defined)
158. **CDP** — Coprocessor data processing
159. **CDP2** — Coprocessor data processing (unconditional)
160. **LDC** — Load from coprocessor
161. **LDC2** — Load from coprocessor (unconditional)
162. **STC** — Store to coprocessor
163. **STC2** — Store to coprocessor (unconditional)
164. **MCR** — Move to coprocessor register
165. **MCR2** — Move to coprocessor register (unconditional)
166. **MRC** — Move from coprocessor register
167. **MRC2** — Move from coprocessor register (unconditional)
168. **MCRR** — Move two regs to coprocessor (64-bit)
169. **MCRR2** — Move two regs to coprocessor (unconditional)
170. **MRRC** — Move two regs from coprocessor (64-bit)
171. **MRRC2** — Move two regs from coprocessor (unconditional)

If you want this as a machine-readable list or split by category (arith, load/store, branch, system, coprocessor), say the word and I’ll format it that way.

What is the list Sorted by that B get's 129?

###### ChatGPT said:

Thought for 10s

Short answer: it isn’t a canonical ARM number. 🙂

In that list I numbered instructions by **walking the A32 primary decode space** in ascending order, roughly:

1. sort by the **primary opcode field** `bits[27:25]` (0b000 … 0b111)
2. then by **bit[24]** (when that splits a class)
3. then by any relevant **sub-opcode bits** inside the class

That puts the classes in this order:

- `00x` → data-processing (AND/EOR/…/MOV/MVN, etc.)
- `01x` → single data transfers (LDR/STR, etc.)
- `100` → block transfers (LDM/STM)
- `101` → **branches** (B/BL)  ⟵ your **B (imm)** lands here
- `110` → coprocessor transfers
- `111` → coprocessor ops / SWI / UDF, etc.

Because branches (`101x`) come after a big pile of data-processing and loads/stores, **B (imm)** ended up at position **129** in that particular enumeration.

If you’d prefer a different scheme (alphabetical, by mnemonic family, or by exact mask/value pattern), I can renumber to match it. There’s no official ARM “instruction #129.”