Question:  If this was a classroom and I was the instructor and I wanted to explain what happens for the ADD command - can you walk me thorugh parts of the fetch./decode/execution or whatever that would happen?

###### ChatGPT said:

Thought for 20s

Totally—here’s a clean, classroom-style walk-through of what happens for a classic ARM **ADD** in ARM state, from fetch → decode → execute → write-back, plus exactly how the PC behaves.

# 1) The instruction format (data-processing)

```
31            28 27 26 25 24 23 22 21 20 19 16 15 12 11                  0
+---------------+--+--+--+----+----+--+-----+-----+-----------------------+
|     cond      | 0| 0| I| opcode   | S|  Rn |  Rd |   shifter_operand    |
+---------------+--+--+--+---------+--+-----+-----+-----------------------+

I=0: operand2 is (Rm with optional shift by imm/reg)
I=1: operand2 is (imm8 rotated right by 2*rotate_imm)
```

For **ADD**, `opcode = 0100b` (0x4). `S` decides whether flags are updated.

# 2) Pipeline view (simple 3-stage mental model)

- **Fetch (F):** Read 32-bit instruction at address **A = PC**.
- **Decode (D):** Parse fields; evaluate the **condition**; build **operand2** (barrel shifter).
- **Execute/Write-back (E/W):** Perform the addition; optionally set flags; write result to `Rd`.

Important PC conventions:

- In ARM state, when an instruction **reads** `r15` (PC) as an operand, it gets **A + 8** (because of the prefetch effect).
- Your emulator’s “stepper” does **`PC = A + 4`** after execute **unless** the instruction already wrote PC.

# 3) Step-by-step on a concrete example

Let’s use:

```
A = 0x00008000
ADD r2, r1, #0x40      ; no flags (S=0), cond=AL (always)
Encoding (schematically): cond=1110, I=1, opcode=0100, S=0, Rn=1, Rd=2, imm8=0x40, rotate=0
```

## Fetch

- Bus reads `Instr` from `mem[0x00008000]`.
- The architectural PC seen by *operands* for this instruction is **A + 8 = 0x00008008**.

## Decode

- **cond** = `AL` → passes, so instruction will execute (otherwise it would behave like a NOP: no side-effects, but PC still steps).
- **I=1** → `operand2` is immediate:
  - `imm8 = 0x40`, `rotate_imm = 0`, so `operand2 = ROR(0x40, 0)` = `0x00000040`.
  - (There is a “carry out of shifter” concept; **for ADD it does not feed C**, only logical ops use shifter C.)
- **Rn = r1** → read `op1 = r1`. If `Rn==r15`, you would use `A + 8`. Here it’s just `r1`.

## Execute

- **result = op1 + operand2** (for ADC, you’d add carry-in; for ADD it’s just +).
- **Flags**:
  - If `S=1` and `Rd != r15`:
    - `N = result[31]`
    - `Z = (result == 0)`
    - `C = carry-out of the addition`
    - `V = signed overflow of the addition`
  - If `S=0`: flags not touched.
  - If `S=1` and `Rd==r15`: special: CPSR is loaded from SPSR (mode-dependent). (You can just call this out as advanced/privileged behavior.)

## Write-back

- Write `result` to `r2`.
- **PC update**:
  - Because **this instruction did not write PC**, your stepper runs after the fact: `PC = A + 4 = 0x00008004`.
  - If the instruction **had** written PC (e.g., `MOV pc, rX`, `LDR pc, [..]`, taken `B/BL/BX`), the stepper must **not** add 4.

# 4) What changes if operands use shifts or registers?

- If `I=0`, `operand2` is `(Rm <<shift)` where `shift` can be by immediate or by `Rs` (register). The **barrel shifter** computes both the shifted value and a **shifter carry-out**.
- For **ADD/ADC/SUB/SBC/…** the **C flag** (when `S=1`) comes from the **ALU add/sub**, **not** from the shifter.
- For logical ops (**AND/EOR/ORR/BIC/MOV/MVN**), when `S=1`, **C** comes from the **shifter carry-out**.

# 5) What changes if r15 is used as an operand or destination?

- **Reading PC**: any source read from `r15` uses **A + 8**.
- **Writing PC**: if `Rd == r15`, you’re doing a control-flow change.
  - Write the computed value to PC and **suppress the stepper** (don’t add 4).
  - If `S=1` and privileged, you also restore CPSR from SPSR (return-from-exception semantics).

# 6) Pseudocode you can show on a slide

```
void exec_ADD(uint32_t instr, uint32_t A /* old PC */) {
    // 1) Decode
    uint8_t cond   = instr >> 28;
    if (!cond_passes(cond)) { cpu.r[15] = A + 4; return; }

    bool S         = (instr >> 20) & 1;
    uint8_t Rn     = (instr >> 16) & 0xF;
    uint8_t Rd     = (instr >> 12) & 0xF;
    bool I         = (instr >> 25) & 1;

    uint32_t op1   = (Rn == 15) ? (A + 8) : cpu.r[Rn];
    uint32_t op2;
    if (I) {
        uint32_t imm8   = instr & 0xFF;
        uint32_t rot    = ((instr >> 8) & 0xF) * 2;
        op2 = ror32(imm8, rot);
    } else {
        op2 = barrel_shifter_operand2(instr, A /* for R15 reads */);
        // shifter_carry ignored for ADD flags
    }

    // 2) Execute
    uint64_t wide  = (uint64_t)op1 + (uint64_t)op2;
    uint32_t res   = (uint32_t)wide;

    // 3) Flags (only if S=1 and Rd!=PC)
    if (S && Rd != 15) {
        set_flag_N(res >> 31);
        set_flag_Z(res == 0);
        set_flag_C((wide >> 32) & 1);                 // carry out
        set_flag_V(add_overflow(op1, op2, res));      // signed overflow
    }

    // 4) Write-back
    if (Rd == 15) {
        cpu.r[15] = res;                 // control flow change
        // if S=1 and privileged: CPSR = SPSR;
    } else {
        cpu.r[Rd] = res;
        cpu.r[15] = A + 4;               // normal stepper
    }
}
```

# 7) One-line takeaway for students

> **ADD** reads its operands (PC reads as *A+8*), sums them in the ALU, optionally updates flags (from the **addition**), writes `Rd`, and the PC either jumps (if `Rd==pc`) or simply advances by 4 (done by the stepper).



So in this example the stepper portion occurs during the execution portion

###### ChatGPT said:

Thought for 4 seconds

Yep—that’s exactly it.

In your emulator model, the “stepper” (i.e., `PC = A + 4`) happens **at the end of execute/write-back** for any instruction that **didn’t** explicitly write `PC`. The simple rules:

1. Take a snapshot `A = old PC` before executing.
2. Execute the instruction.
   - If it **writes PC** (B/BL/BX, taken conditional B, `LDR pc, [...]`, `MOV pc, ...`, etc.), **do not** step.
   - If the **condition fails**, do nothing else, but you’ll still step.
3. If PC wasn’t written, do the stepper: **`PC = A + 4`**.

Separate but related: whenever an instruction **reads** `r15` as an operand, it must see **`A + 8`** (prefetch effect), independent of the stepper.

So yes—conceptually the stepper runs at the tail end of “execute/write-back,” and you suppress it only when control-flow wrote PC.