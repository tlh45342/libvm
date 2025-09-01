what is the SIMH method for turning on debug trace



###### ChatGPT said:

In SIMH (a simulator for historical computer systems), enabling **debug trace** is a way to get detailed information about the internal operation of simulated devices or the simulator itself.

### To turn on debug trace in SIMH:

1. **Start SIMH** (e.g., running a PDP-11 simulation).
2. Use the **`SET`** command to enable debugging for a specific device.
3. Use the **`SET DEBUG`** command to specify the debug output file (optional).

------

### Example Commands

```
bashCopyEditsim> SET CPU DEBUG
sim> SET CPU DEBUG=ALL      ; enables all debug flags for CPU
sim> SET CPU NODEBUG        ; turns off debug for CPU
```

To specify a debug output file:

```
bash


CopyEdit
sim> SET DEBUG LOG=debug.log
```

This tells SIMH to write all debug output to `debug.log`.

------

### Show Available Debug Options

You can list debug flags for a device (e.g., CPU):

```
bash


CopyEdit
sim> SHOW CPU DEBUG
```

This will show which debug flags are currently enabled and available.

------

### Example for a PDP-11 Simulator

```
bashCopyEditsim> SET CPU DEBUG=INTR,TRAP,EXEC
sim> SET DEBUG LOG=trace.log
```

This will log interrupt, trap, and execution traces for the CPU to `trace.log`.

------

### Notes

- The actual debug flags (like `INTR`, `EXEC`, `IO`, etc.) vary depending on the simulated device.
- Debugging can slow down the simulation significantly.
- You can use `HELP SET DEBUG` in SIMH for more guidance.

------

Let me know which system (e.g., PDP-11, VAX, etc.) you're simulating if you want flag suggestions tailored to it.



##### You said:

what is the difference between "set cpu  debug" and "set CPU debug=all" in simh?



###### ChatGPT said:

In SIMH, the commands:

- `SET CPU DEBUG`
- `SET CPU DEBUG=ALL`

**do different things**, even though they seem similar.

------

### ✅ `SET CPU DEBUG`

This command:

- **Enables debugging** for the CPU **without enabling any specific debug flags**.
- It **marks the CPU device as being in debug mode**, so SIMH will write debug output **only if debug flags are set** later.

In short:
 🟡 It **activates debugging**, but **no actual output** will appear unless you also enable specific debug flags (like `EXEC`, `INTR`, etc.).

------

### ✅ `SET CPU DEBUG=ALL`

This command:

- **Enables debugging for the CPU**, **and**
- **Turns on all available debug flags** for the CPU.

So this **produces maximum debugging output** for that device.

In short:
 🟢 It activates debugging **and** outputs **everything** the CPU can log.

------

### Summary Table

| Command             | What it does                            | Produces debug output?       |
| ------------------- | --------------------------------------- | ---------------------------- |
| `SET CPU DEBUG`     | Enables debug mode, but no flags are on | ❌ (not unless flags are set) |
| `SET CPU DEBUG=ALL` | Enables debug mode and sets all flags   | ✅ (full output)              |



------

### Good Practice

- Use `SHOW CPU DEBUG` to see current flags and available options.

- You can turn off debugging entirely with:

  ```
  bash
  
  
  CopyEdit
  SET CPU NODEBUG
  ```

Let me know if you'd like help selecting specific debug flags (e.g., only I/O, instruction execution, etc.).



##### You said:

What are my options for SET CPU DEBUG=  if I don't choose all?  What are my options?



###### ChatGPT said:

Here’s how SIMH’s `SET CPU DEBUG=` works when **not** using `ALL`:

------

### 🛠️ General behavior (all devices)

- When a device supports **multiple named debug flags**, `SET <dev> DEBUG` (without `=…`) enables **all flags**.
- To turn on **specific flags**, use `SET <dev> DEBUG=flag1;flag2;…`.
- Similarly, `SET <dev> NODEBUG=flag1;flag2` disables only specified flags, while plain `SET <dev> NODEBUG` disables all. [ibm-1401.info+10manualzz.com+10magicandroidapps.com+10](https://manualzz.com/doc/10844568/simh-simulator-users’-guide?utm_source=chatgpt.com)[Open SIMH](https://opensimh.org/simdocs/simh_doc.html?utm_source=chatgpt.com)

------

### ⚙️ CPU-specific flags — varies by CPU type

Unfortunately, the available flags depend on the simulated CPU (e.g. PDP‑11, VAX, Altair Z‑80). For most platforms, SIMH doesn't publish a universal list — you'd use `HELP CPU SET` or `SHOW CPU DEBUG` in the simulator to see them.

However, for **Altair Z‑80** (as an example), documented CPU‑device debug flags include:

- `LOG_IN` — logs all IN port operations
- `LOG_OUT` — logs all OUT port operations
   (useful when debugging I/O instructions) [StudFiles+1magicandroidapps.com+1](https://studfile.net/preview/2973467/?utm_source=chatgpt.com)

If only those two flags exist:

```
textCopyEditSET CPU DEBUG=LOG_IN           ; only IN logging
SET CPU DEBUG=LOG_OUT          ; only OUT logging
SET CPU DEBUG=LOG_IN;LOG_OUT   ; both
SET CPU DEBUG                    ; equivalent to ALL
```

------

### ✅ Example Usage and What It Means

| Command                        | Outcome                               |
| ------------------------------ | ------------------------------------- |
| `SET CPU DEBUG=LOG_IN`         | Only logs IN instructions             |
| `SET CPU DEBUG=LOG_OUT`        | Only logs OUT instructions            |
| `SET CPU DEBUG=LOG_IN;LOG_OUT` | Logs both IN and OUT                  |
| `SET CPU DEBUG` or `=ALL`      | Enables all available CPU debug flags |
| `SET CPU NODEBUG=LOG_IN`       | Turns off only `LOG_IN`               |
| `SET CPU NODEBUG`              | Disables all CPU debug output         |



------

### 📌 How to discover available flags yourself

At the `sim>` prompt:

- Run `SHOW CPU DEBUG` to see which flags are active (and possibly which ones are available).
- Run `HELP CPU SET` (or `HELP <dev> SET`) to display possible SET modifiers including debug flags. [Reddit+2Open SIMH+2Reddit+2](https://opensimh.org/simdocs/simh_doc.html?utm_source=chatgpt.com)[magicandroidapps.com](https://magicandroidapps.com/simh/doc/altairz80/?utm_source=chatgpt.com)[Open SIMH+1Reddit+1](https://opensimh.org/simdocs/vax_doc.html?utm_source=chatgpt.com)

------

### 🧩 TL;DR

- Without `=`, `SET CPU DEBUG` defaults to enabling **all flags**.
- With `=flag1;flag2`, you can pick exactly which aspects (e.g. I/O, memory access, traps) you want logged.
- Flag names vary by CPU type — use `SHOW` and `HELP` to explore.

If you let me know which specific CPU you're using (like PDP‑11 or VAX), I can help list the exact available debug flags for that configuration.



##### You said:

if I say "set cpu debug" and then say "step" will it then display the decoded instruction?



###### ChatGPT said:

Great question — and the answer depends on **which debug flags** are enabled.

### ✅ Short Answer:

**No**, just running:

```
textCopyEditsim> SET CPU DEBUG
sim> STEP
```

**will not display the decoded instruction**, unless the appropriate debug flag (like `EXEC`) is also set.

------

### 🧠 Why?

- `SET CPU DEBUG` **enables debugging**, but **does not automatically turn on any specific debug output**, unless the simulator treats it as equivalent to `DEBUG=ALL` (which is **not** consistent across all SIMH systems).
- The `STEP` command simply executes a single instruction — if the relevant debug flag is active (usually `EXEC` or `INSTR`), then you'll see decoded instruction info.

------

### 🧪 To see decoded instructions during stepping:

Run:

```
textCopyEditsim> SET CPU DEBUG=EXEC
sim> STEP
```

Or, to cover all bases:

```
textCopyEditsim> SET CPU DEBUG=ALL
sim> STEP
```

Now you'll typically see output like:

```
text


CopyEdit
[CPU   ] 004000: MOV R1,R2
```

This shows the address and the decoded instruction (e.g., for PDP-11 or VAX).

------

### 🧰 Pro Tip:

To see available debug flags for your CPU:

```
text


CopyEdit
sim> SHOW CPU DEBUG
```

And:

```
text


CopyEdit
sim> HELP CPU SET
```

Some CPUs use flags like:

- `EXEC` — for instruction execution
- `INSTR` — for disassembled instruction traces
- `TRACE`, `FLOW`, `FETCH`, etc. — varies by CPU

------

Let me know what CPU you're simulating (e.g., PDP-11, VAX, Z80), and I can give you the exact flag to see decoded instructions.



##### You said:

Then what is the difference between "EXEC" and "INSTR"?



###### ChatGPT said:

Great question — the difference between `EXEC` and `INSTR` in SIMH debug flags is subtle and **depends on the simulated CPU**, but here’s the general distinction:

------

### 🔍 **EXEC** – *Execution Trace*

- Traces **execution of CPU instructions**.
- Typically logs:
  - Program counter (PC)
  - Raw instruction opcode
  - Register changes (in some systems)
- May **not** always show the **decoded** instruction (e.g., mnemonic like `MOV` or `ADD`).

#### Example output:

```
css


CopyEdit
[CPU   ] 004000: 012702         ; raw opcode only
```

------

### 🔍 **INSTR** – *Disassembled Instruction Trace*

- Shows the **decoded** (disassembled) instruction.
- Often includes:
  - Address
  - Instruction mnemonic (like `MOV`, `ADD`)
  - Operands
- This is what you usually want when **stepping** or analyzing flow.

#### Example output:

```
csharp


CopyEdit
[CPU   ] 004000: MOV #2, R2
```

------

### 🧠 Summary of Differences

| Debug Flag | Focus                             | Shows Disassembly? | Shows Raw Opcode? | Typical Use                          |
| ---------- | --------------------------------- | ------------------ | ----------------- | ------------------------------------ |
| `EXEC`     | Instruction **execution trace**   | ❌ (not always)     | ✅                 | Low-level debugging, opcode tracking |
| `INSTR`    | Instruction **disassembly trace** | ✅                  | ✅ or ❌            | High-level tracing, readable output  |



------

### ✅ Best Practice

If you're stepping through code and want to **see what the CPU is doing in human-readable form**, use:

```
text


CopyEdit
sim> SET CPU DEBUG=INSTR
```

Or, if unsure:

```
text


CopyEdit
sim> SET CPU DEBUG=ALL
```

Then:

```
text


CopyEdit
sim> STEP
```

------

Let me know what CPU you're simulating (e.g., PDP-11, VAX, Z80), and I can give you the exact flag behavior for that system.