what additional instructions would you focus on?

###### ChatGPT said:

Thought for 39s

Great question. Now that basic DP/branches/word loads/stores and UART work, I’d focus on the stuff that compiled C code hits most. Here’s a tight, high-impact order:

1. **Finish the common DP/flag surface (low effort, big payoff)**
   - Wire decoders for **ADC/SBC/RSB** (you already have `exec_arith` support—just match them in `execute()`).
   - Add missing logicals:
     - **EOR/ORR/BIC**, both imm and reg (your AND path is a good template).
     - **MVN (reg)**, **CMN**, **TEQ**, **TST (reg)**.
   - Ensure **shifter carry-out** is used for the S-variants (you already have `dp_operand2` doing the right thing).
2. **Halfword + signed loads (very common)**
   - **LDRH/STRH**, **LDRSH/LDRSB** (table-driven decode is easiest).
   - Keep them unaligned-tolerant like your 32-bit paths (assemble bytes LE).
3. **Multiply family (shows up in libgcc and hot code)**
   - **MUL/MLA**, then **UMULL/SMULL** (+S variants set NZ).
   - (Optional later) **UDIV/SDIV** (ARMv7-A/R); if absent on target, libgcc will emulate anyway.
4. **Bit twiddling used by optimized code**
   - **UBFX/SBFX**, **BFI/BFC** (bitfield extract/insert).
   - **CLZ**.
   - **REV/REV16/REVSH** (byte-swap helpers).
5. **Load/store multiple: all addressing modes**
   - Your `LDM/STM` currently behaves like **FD/DB** only. Support IA/IB/DA/DB, write-back, and **PUSH/POP** aliases fully.
   - Special case: **LDM{...}^** (restore CPSR) can be deferred unless you’re running OS code.
6. **Branch variants**
   - **BLX (imm/reg)**: treat like BL and keep T=0 (you already clear Thumb), update LR per spec.
7. **Atomics: choose one path for now**
   - Easiest: **SWP/SWPB** (deprecated but trivial: atomic read+write).
   - More modern: **LDREX/STREX** with a simple reservation on an address (per-CPU single word). You can start by always succeeding `STREX` (return 0) until you model contention.
8. **Exception/IRQ skeleton (unlocks timers/driver demos)**
   - Minimal **IRQ entry/return** path (bank R13/R14 for IRQ, `SPSR_irq`, vector at 0x18, `SUBS pc, lr, #4` to return).
   - Then add a tiny **timer** device that ticks from `hw_bus_tick()` and asserts IRQ.