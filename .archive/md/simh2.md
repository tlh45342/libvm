 what is the difference betwee SET CPU DEBUG and SET CPU DEBUG=TRACE ?



###### ChatGPT said:

Great question — the difference between `SET CPU DEBUG` and `SET CPU DEBUG=TRACE` in **SIMH** is subtle but important, and it depends slightly on the specific simulator (e.g. PDP-11, VAX, etc.). Here's a breakdown:

------

### 🧩 `SET CPU DEBUG`

This command:

- **Enables general debugging** for the CPU device.
- **Does not specify what to debug** — it simply turns on the debug system.
- SIMH will not output anything unless you also specify one or more **debug flags** (e.g. `TRACE`, `READ`, `WRITE`, etc.).

🔧 It's like flipping on a master switch — but without telling SIMH *what* to debug.

------

### 🧪 `SET CPU DEBUG=TRACE`

This command:

- Enables **debug output specifically for instruction tracing** (i.e., logs each instruction executed).
- TRACE is a **debug flag** that tells SIMH to log instruction-level execution, including addresses and opcodes.
- Often used with `SET CPU DEBUG=LOG=filename` to store output in a file.

✅ This is the **specific** debug mode you're looking for if you want to **log or see instructions as they execute**.

------

### 🧠 Think of It Like This:

| Command               | What it Does                                               |
| --------------------- | ---------------------------------------------------------- |
| `SET CPU DEBUG`       | Turns on debug system (but doesn’t log anything by itself) |
| `SET CPU DEBUG=TRACE` | Turns on instruction-level tracing (logs instructions)     |



To see available debug flags:

```
simh


CopyEdit
SHOW CPU DEBUG
```

You might see something like:

```
rustCopyEditDebugging enabled
Debug flags: TRACE, INTR, IO, etc.
```

------

### 📝 Example (Logging Instructions)

```
simhCopyEditSET CPU DEBUG          ; Enable debug system
SET CPU DEBUG=TRACE    ; Log instruction-level tracing
SET CPU DEBUG=LOG=trace.log  ; Send to file
```

Then `RUN`, `STEP`, or `GO`, and check `trace.log`.