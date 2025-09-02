What does Unicorn or other emulators do in this situation?

###### ChatGPT said:

Thought for 26s

Great question. Short version:

- **Unicorn** is built on **QEMU**. QEMU doesn’t interpret each instruction with mask checks every time; it **decodes once** into TCG IR and emits a tiny block of native code (“translation block”). That code already has all operands extracted (e.g., S, Rn, Rd, op2, sh-carry), so there are **no per-op wrappers** in the hot path—just the translated semantics. Condition codes are compiled in (guards or conditional moves), and the barrel shifter is folded at decode time.
- **Many cycle-light interpreters** (mGBA’s ARM core, older Skyeye-style cores, MAME cores, etc.) do a **table-driven decode** with mask/value matches, then **call a semantic function with decoded operands**. They usually avoid one-wrapper-per-op by either:
  1. a **single DP trampoline** that extracts fields and calls `handle_ADD/EOR/...`, or
  2. a **uniform “decoded operands” struct** that every handler takes, so the table can point **directly** at the final handler.

What to copy in your codebase:

### If you stay interpreter (no JIT)

- Do **Option A** from earlier (one tiny `k12_dp_CALL` dispatcher). It’s simple, fast enough, and kills all the per-op wrappers.
- Or, go one notch closer to “what big emulators do” and pass a **decoded struct** to handlers so you never re-extract fields in the handler:

```
typedef struct {
    uint32_t instr;
    uint8_t  cond;
    uint8_t  opcode;  // DP opcode 0..15
    uint8_t  S, Rn, Rd;
    uint32_t op2;
    uint32_t sh_carry;
} Decoded;

typedef bool (*sem_fn)(const Decoded* d);

// in try_decode_key12(), when you match a row that’s DP or needs fields:
Decoded d = {
    .instr = instr,
    .cond = (instr >> 28) & 0xF,
    .opcode = (instr >> 21) & 0xF,
    .S  = (instr >> 20) & 1u,
    .Rn = (instr >> 16) & 0xFu,
    .Rd = (instr >> 12) & 0xFu,
};
d.sh_carry = cpsr_get_C();
d.op2 = dp_operand2(instr, &d.sh_carry);

// Then call the exact handler (no wrapper):
return e->sem(&d);
```

Your DP table rows can then be:

```
{ K12_MASK_DP_OPCODE, K12_VAL_DP(0x5), 0,0, true, /*ctype=SEM*/, .sem=sem_ADC, "ADC" }
```

…and `sem_ADC` would simply use `d->S, d->Rd, cpu.r[d->Rn], d->op2, d->sh_carry` to call your existing ALU helpers (or inline them).

### If you eventually want Unicorn/QEMU-style speed

- Consider a tiny **decode cache**: map PC→decoded record (or short micro-op). On self-modifying code or writes to code pages, invalidate. This keeps your table-driven clarity while avoiding repeated bit-extractions.
- The next step beyond that is a **mini-IR** or a **tiny threaded-code/JIT**, but that’s optional for now.

### Bottom line

Unicorn/QEMU avoid the issue by **not interpreting**—they compile blocks and bake operands in. Interpreters that aim for clarity/perf typically **decode once per instruction** (at least per execution) and call **direct semantic handlers**—no per-op wrappers. For you, the quickest win is the **single DP trampoline** (or the **Decoded struct**) and pointing table entries **directly** to the final handlers.