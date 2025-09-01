what is the full ARMv7-A instruction table?



###### ChatGPT said:

Alright — here’s the **full ARMv7-A AArch32 (ARM state) instruction set** laid out in categories, like in the ARM ARM (“ARM Architecture Reference Manual”).
 I’ll stick to the **base ARM mode**, not Thumb, NEON, or coprocessor-specific stuff, and not the optional Jazelle/ThumbEE instructions.

------

## **1. Data-processing (register, immediate, shifted forms)**

| Mnemonic | Description                               |
| -------- | ----------------------------------------- |
| **MOV**  | Move                                      |
| **MVN**  | Move Not                                  |
| **ADD**  | Add                                       |
| **ADC**  | Add with Carry                            |
| **SUB**  | Subtract                                  |
| **SBC**  | Subtract with Carry                       |
| **RSB**  | Reverse Subtract                          |
| **RSC**  | Reverse Subtract with Carry               |
| **AND**  | Bitwise AND                               |
| **BIC**  | Bit Clear                                 |
| **ORR**  | Bitwise OR                                |
| **EOR**  | Bitwise Exclusive OR                      |
| **TST**  | Test (AND, no result written)             |
| **TEQ**  | Test Equivalence (EOR, no result written) |
| **CMP**  | Compare (SUB, no result written)          |
| **CMN**  | Compare Negative (ADD, no result written) |



------

## **2. Shift and rotate**

*(These also appear as operand2 in data-processing instructions)*

| Mnemonic | Description              |
| -------- | ------------------------ |
| **LSL**  | Logical Shift Left       |
| **LSR**  | Logical Shift Right      |
| **ASR**  | Arithmetic Shift Right   |
| **ROR**  | Rotate Right             |
| **RRX**  | Rotate Right with Extend |



------

## **3. Multiply and multiply-accumulate**

| Mnemonic   | Description                             |
| ---------- | --------------------------------------- |
| **MUL**    | Multiply                                |
| **MLA**    | Multiply Accumulate                     |
| **MLS**    | Multiply Subtract                       |
| **UMULL**  | Unsigned Multiply Long                  |
| **UMLAL**  | Unsigned Multiply Accumulate Long       |
| **SMULL**  | Signed Multiply Long                    |
| **SMLAL**  | Signed Multiply Accumulate Long         |
| **SMMUL**  | Signed Most Significant Word Multiply   |
| **SMULxy** | Signed Multiply Halfwords (various x/y) |
| **SMLAxy** | Signed Multiply Accumulate Halfwords    |



------

## **4. Load/store (byte, halfword, word, doubleword)**

| Mnemonic  | Description          |
| --------- | -------------------- |
| **LDR**   | Load Word            |
| **STR**   | Store Word           |
| **LDRB**  | Load Byte            |
| **STRB**  | Store Byte           |
| **LDRH**  | Load Halfword        |
| **STRH**  | Store Halfword       |
| **LDRSB** | Load Signed Byte     |
| **LDRSH** | Load Signed Halfword |
| **LDRD**  | Load Doubleword      |
| **STRD**  | Store Doubleword     |
| **SWP**   | Swap Word            |
| **SWPB**  | Swap Byte            |



------

## **5. Load/store multiple**

| Mnemonic                                                     | Description    |
| ------------------------------------------------------------ | -------------- |
| **LDM**                                                      | Load Multiple  |
| **STM**                                                      | Store Multiple |
| Variants: IA (increment after), IB (increment before), DA (decrement after), DB (decrement before), with optional `^` for user-mode access. |                |



------

## **6. Branch and control**

| Mnemonic | Description                    |
| -------- | ------------------------------ |
| **B**    | Branch                         |
| **BL**   | Branch with Link               |
| **BX**   | Branch and Exchange            |
| **BLX**  | Branch with Link and Exchange  |
| **BKPT** | Breakpoint                     |
| **SVC**  | Supervisor Call (formerly SWI) |
| **HVC**  | Hypervisor Call                |
| **SMC**  | Secure Monitor Call            |



------

## **7. Status register access**

| Mnemonic | Description          |
| -------- | -------------------- |
| **MRS**  | Move PSR to Register |
| **MSR**  | Move Register to PSR |



------

## **8. Saturating arithmetic (optional, but common in DSP profiles)**

| Mnemonic             | Description                    |
| -------------------- | ------------------------------ |
| **QADD**, **QSUB**   | Saturating add/subtract        |
| **QDADD**, **QDSUB** | Saturating double add/subtract |



------

## **9. Miscellaneous data-processing**

| Mnemonic             | Description                                 |
| -------------------- | ------------------------------------------- |
| **CLZ**              | Count Leading Zeros                         |
| **REV**              | Reverse Byte Order in Word                  |
| **REV16**            | Reverse Byte Order in Each Halfword         |
| **REVSH**            | Reverse Byte Order in Halfword, Sign-Extend |
| **UXTB**, **UXTH**   | Unsigned Extend Byte/Halfword               |
| **SXTB**, **SXTH**   | Signed Extend Byte/Halfword                 |
| **UXTAB**, **UXTAH** | Unsigned Extend and Add                     |
| **SXTAB**, **SXTAH** | Signed Extend and Add                       |



------

## **10. Synchronization and memory barriers**

| Mnemonic  | Description                         |
| --------- | ----------------------------------- |
| **DMB**   | Data Memory Barrier                 |
| **DSB**   | Data Synchronization Barrier        |
| **ISB**   | Instruction Synchronization Barrier |
| **NOP**   | No Operation                        |
| **SEV**   | Send Event                          |
| **WFE**   | Wait For Event                      |
| **WFI**   | Wait For Interrupt                  |
| **YIELD** | Hint to scheduler                   |



------

## **11. Coprocessor/system**

*(CP15 register access, system instructions — only if you emulate privileged mode)*

| Mnemonic      | Description                            |
| ------------- | -------------------------------------- |
| **MCR/MRC**   | Move to/from Coprocessor               |
| **MCRR/MRRC** | Move Two Registers to/from Coprocessor |
| **CDP**       | Coprocessor Data Processing            |
| **LDC/STC**   | Load/Store to Coprocessor              |



------

That’s the **core ARMv7-A ARM mode table**.
 The ARM ARM groups them into about **10 categories** with encoding diagrams.



Can you explain HVS and SMC calls



###### ChatGPT said:

Sure — both **HVC** (often you see “HVS” as shorthand for HyperVisor call) and **SMC** are *privileged* ARMv7-A/AArch32 instructions used to jump between **exception levels** (worlds/modes), kind of like **SVC** but aimed at virtualization and security firmware instead of the OS kernel.

------

## **1. SVC vs HVC vs SMC — the big picture**

- **SVC** (Supervisor Call) — traps from **EL0 (user mode)** to **EL1 (kernel)**. Used for syscalls.
- **HVC** (HyperVisor Call) — traps from **EL1 (kernel)** to **EL2 (hypervisor)**. Used for hypercalls in virtualized systems.
- **SMC** (Secure Monitor Call) — traps from **Normal World** into **Secure Monitor** (TrustZone). Used to request secure services.

------

## **2. HVC — Hypervisor Call**

**Mnemonic:** `HVC #imm`
 **Purpose:** Provide a controlled entry into the hypervisor (EL2) from an OS or guest kernel (EL1).
 **Use cases:**

- Ask hypervisor to perform device emulation or privileged ops on behalf of guest OS.
- VCPU scheduling, memory map changes, interrupt injection.

**Execution flow (ARMv7-A Virtualization Extensions)**:

1. Guest kernel executes `HVC` with an immediate code (0–65535).
2. CPU takes a **Hyp Trap** exception to EL2.
3. Hypervisor reads the immediate and the general-purpose registers to know what service is requested.
4. Hypervisor returns to guest OS with `ERET`.

**Privilege rules:**

- Only allowed at PL1 (kernel) or higher.
- If executed without Virtualization Extensions or at the wrong mode, usually causes `UNDEFINED`.

------

## **3. SMC — Secure Monitor Call**

**Mnemonic:** `SMC #imm`
 **Purpose:** Switch between **Normal World** and **Secure World** in TrustZone systems.
 **Use cases:**

- Request secure firmware functions (crypto key handling, secure storage, firmware update).
- Switch into “secure OS” to run code that handles secrets.

**Execution flow:**

1. CPU in Normal World executes `SMC` with an immediate code.
2. CPU takes a **Monitor Mode** exception and enters **Secure Monitor** (highest privilege in Secure World).
3. Monitor inspects the call, may pass to Secure World OS, or back to Normal World.
4. Return to caller with `MOVS pc, lr` or `ERET` (depending on architecture).

**Privilege rules:**

- Usually called from PL1 (kernel) in Normal World.
- If executed at EL0 or without monitor mode, causes `UNDEFINED`.

------

## **4. Why they exist separately**

- **SVC** is for OS syscalls — user space → kernel.
- **HVC** is for virtualization — guest OS → hypervisor.
- **SMC** is for TrustZone — normal world → secure monitor.

They’re separate opcodes because:

- They vector to different exception handlers.
- The CPU’s current security state (Normal/Secure) and virtualization state determine how they’re handled.
- They can be trapped/filtered independently.

------

## **5. In your emulator context**

If you’re just doing a **bare-metal ARMv7-A** VM without TrustZone or virtualization extensions:

- **SVC**: easy to emulate — you probably already do (syscall dispatcher).
- **HVC**: can trap as “unimplemented” or `UNDEFINED` unless you want to simulate a hypervisor mode.
- **SMC**: can also trap or be ignored unless you build a TrustZone-like monitor.

If you eventually want to boot **a guest OS under a hypervisor** in your VM, you’d emulate HVC as a software trap to your EL2 logic.
 If you want to explore **secure monitor mode**, you’d emulate SMC to jump into a separate “secure” CPU context.

------

If you want, I can give you **flow diagrams** showing exactly where in the ARMv7 exception vector table HVC and SMC land, and what registers get saved on entry/exit. That would help if you’re thinking of actually trapping these in your emulator. Do you want me to draw that up?