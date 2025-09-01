What I am working with here is that the PC step is organically part of the execution; even if it s at the later half. My worry is that by introducing another variable and introducing testing and examination of said variable that I am increasing execution time overall.  I am trying to examine if it's better off just to handle the PC manipulation in the execution portion.  Presumably we know there as part of the tests conducted there what we need to do with the PC.  In other words the question comes down to by introducing and managing this tracker; is this going to introduce overhead that effects the overall performance of the execution.  And by comparison would applying the execution inside the command (at some convenient place (even if at the end)) would that not be the OPTIMAL and truest representation of what goes on.  We agree that we repeat the step many time; and at the end of the execution; but in this way don't we stay truer AND avoid additional management overhead?

###### ChatGPT said:

Thought for 22s

Totally fair concern—and you can get both correctness *and* zero “tracker” overhead.

### TL;DR (recommended pattern)

Use a **local `next_pc`** in the execute step, initialized to `A + 4`. Handlers that change control flow just **write `next_pc`**, and nobody writes `cpu.r[15]` directly. At the end of execute, **commit once**: `cpu.r[15] = next_pc`. No booleans, no extra branches, no double-writes.

Why this wins:

- **No flag/branch** at the end (better for branch prediction).
- **Single store** to PC per instruction.
- **Centralized & audit-friendly** (all PC changes flow through one line).
- **“True to hardware”**: default fall-through unless an instruction overrides it.

### Sketch

```
void execute_step(void) {
    uint32_t A       = cpu.r[15];       // fetch address (old PC)
    uint32_t instr   = mem_read32(A);   // fetch
    uint32_t next_pc = A + 4;           // default sequencer

    // decode uses A if it needs PC-relative operands (remember PC reads as A+8)
    decoded_t d = decode(instr, A);

    switch (d.opcode) {
      case OPC_B:
        if (cond_passed(d.cond)) {
            int32_t off = ((int32_t)(instr & 0x00FFFFFF) << 8) >> 6; // imm24<<2 sign-extended
            next_pc = (A + 8) + (uint32_t)off;
        }
        break;

      case OPC_BL:
        if (cond_passed(d.cond)) {
            cpu.r[14] = A + 4;
            int32_t off = ((int32_t)(instr & 0x00FFFFFF) << 8) >> 6;
            next_pc = (A + 8) + (uint32_t)off;
        }
        break;

      case OPC_BX:
        if (cond_passed(d.cond)) {
            uint32_t addr = cpu.r[d.rm];
            next_pc = addr & ~1u;  // (ignore Thumb for now)
        }
        break;

      // Example: LDR (pc)
      case OPC_LDR:
        {
          uint32_t val = load(...);
          if (d.rd == 15) {
            next_pc = val & ~1u;   // pipeline flush target
          } else {
            cpu.r[d.rd] = val;
          }
        }
        break;

      // Normal ALU: never touch cpu.r[15] directly here
      case OPC_ADD:
        cpu.r[d.rd] = alu_add(...);
        break;

      // etc...
    }

    cpu.r[15] = next_pc;   // single commit, no conditionals
}
```

### Compared to other designs

- **“pc_written” boolean + conditional stepper**
   Works, but adds a branch in the hot path (`if (!pc_written) pc=A+4;`). Usually well-predicted, but still extra control-flow and a second conceptual state to maintain.
- **Handlers write `cpu.r[15]` directly**
   Then your stepper must *not* clobber it, which forces a flag anyway (or duplicating logic in every handler). It’s easy to miss edge cases (e.g., `MOV pc, reg`, `LDR pc, [...]`, `SUBS pc, lr, #…`).

The **`next_pc`-only** approach keeps the hot loop simple: one local variable (likely lives in a register), straight-line code, and one store to PC. In practice, the overhead difference vs. a boolean is noise compared to decode, memory, and device accesses—but the `next_pc` pattern is both cleaner and typically faster (no extra branch, fewer writes).

If you want, I can adapt your branch/ALU handlers to this pattern (void functions that take `A` and a `uint32_t* next_pc`) so you don’t add any global state or returns.

##### You said:

Is there a catch phrase of mnemonic that can refer to this styling so that I can reference it quickly and we both know what we are talking about?

###### ChatGPT said:

Thought for 7s

Here are a few snappy options—pick one and we’ll use it everywhere:

- **Next-PC Commit (NPC)** — default `next_pc=A+4`, control-flow overrides, commit once at end.
- **PC Writeback at End (PC-WB)** — PC is written exactly once during writeback.
- **Single-Commit PC (SCP)** — one store to PC per instruction.
- **Deferred PC Commit (DPC)** — compute target, defer write until finish.
- **One-Writer PC Rule** — only the stepper writes PC, handlers set `next_pc`.
- **Fallthrough-Unless** — fallthrough by default; branches just change `next_pc`.

My vote: **Next-PC Commit (NPC)**. Easy to remember and precise.

Suggested file header blurb:

```
// NPC pattern: next_pc = A + 4; control-flow ops set next_pc; at end do cpu.r[15] = next_pc.
```