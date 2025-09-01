A few more questions.....should the CPU handle the abstraction or should we handle the printing of the word outside at the VMRUN or VMSTEP Level and then only if it's enabled?  I am trying to figure out where the code REALLY should be at that prints this.  Once we have that in play we can build a SOLID foundation that begins tos LOG with more of an OBJDUMP style canotation.  This will also allow me to build more stable tests based on just FINDING the 32instr

###### ChatGPT said:

Thought for 17s

Short answer: keep the **CPU core pure**. Do the pretty “objdump-style” printing **outside** the core—at the VM step/run layer—guarded by a debug flag. The CPU should expose *data* (PC, fetched instr, maybe a decode summary) but not formatting or I/O.

Here’s a clean, stable split that works great for tests:

# What goes where

- **CPU core (execute.c / cpu.c)**
  - Fetch the 32-bit word at PC.
  - Execute it.
  - Increment cycle counters, update CPSR, etc.
  - **No I/O formatting**. At most call a *callback* or return metadata.
- **VMSTEP/VMRUN layer (vm.c / cli.c)**
  - If tracing is enabled, call a **disassembler** helper to turn `(pc, instr)` into the exact `objdump`-style line.
  - Print the line with your logger.
- **Disassembler helper (new tiny module)**
  - Pure function: `(pc, instr) -> "    8014:       e3e05000        mvn     r5, #0"`.
  - No side effects. Easy to unit-test.
  - Reuse your K12 patterns to choose mnemonic & operands









## Examples that will now work

- Numeric:

```
set debug=0x00000041              ; INSTR | OBJDUMP
set cpu debug=65                  ; same as above (decimal)
```

- Named:

```
set debug=INSTR,OBJDUMP
set cpu debug=ALL
set cpu debug=NONE
set cpu debug=INSTR|MMIO+DISK
```