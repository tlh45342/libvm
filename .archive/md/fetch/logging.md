speaking of which I see we are calling it semantic logging and ?  What are we naming the other style of logging

###### ChatGPT said:

Thought for 24s

I’d call them:

- **Semantic logging** — human-readable “what the instruction means.”
   e.g. `[ADD imm] r2 = r1 + #0x7 => 0x0000000C`
- **Micro logging** (aka **ALU/flags** or **state-delta** logging) — low-level effect details from the handler.
   e.g. `[ADD] r2: 0x00000005 -> 0x0000000C (a=0x00000005, op2=0x00000007, C=0 V=0)`

If you want clean switches, use three bits:

```

```