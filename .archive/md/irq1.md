what's going to be a good routine to use that uses the IRQ that we can test or prove with?

###### ChatGPT said:

Thought for 1m 8s

Two great, *provable* IRQ demos—pick the one that fits your next steps:

## 1) UART RX “echo” interrupt (easiest, super-visible)

**Why it’s great:** no new device needed beyond UART; you can inject bytes from the host and instantly see the handler run.

**What it does:** whenever a byte arrives in UART RX, an IRQ fires; the handler reads `DR` and writes it back to `DR` (TX), so whatever you type/inject is echoed.

**Pieces to add:**

- **CPU:** IRQ entry/exit
  - On pending IRQ and `I==0`: save `CPSR`→`SPSR_irq`, set mode=IRQ (0x12), `I=1`, `T=0`, set `LR_irq = PC`+4 (or your chosen next-PC), `PC = 0x00000018`.
  - Return with `SUBS pc, lr, #4`.
- **UART device:** minimal IRQ logic
  - RX FIFO + status bit; when RX goes non-empty: `RIS=1`; if IRQ enabled: raise IRQ line.
  - Clear-on-read or via a small `ICR` register write.
- **Vectors/firmware:** vector at `0x18` branches to `irq_handler`; main enables IRQ (`MSR CPSR_c, ...` clear I bit) and loops `WFI`.

**Example handler (ARM A32, pseudo-asm):**

```
irq_handler:
    ldr   r1, =UART_BASE        @ 0x09000000
    ldrb  r0, [r1, #0]          @ DR: read byte
    str   r0, [r1, #0]          @ DR: write byte back (TX)
    ldr   r2, =UART_ICR         @ if you have one; else skip
    mov   r3, #1
    str   r3, [r2]              @ clear UART IRQ
    subs  pc, lr, #4            @ return from IRQ
```

**How to test:**

- Start the VM with this firmware loaded (vectors at 0x0, code anywhere).
- From host/CLI, inject bytes: e.g. `uart inject "ping\n"` (or via your `dev_uart_inject_rx()` helper).
- Expected: the same “ping↵” appears on the VM console (file/stdout) one byte per IRQ.

------

## 2) Timer “tick” printer (slightly more setup, very deterministic)

**Why it’s great:** exercises `hw_bus_tick(cycles)`, an MMIO device, and IRQ path—perfect to validate your timing model.

**What it does:** a down-counter device asserts IRQ at underflow; the handler prints one character per tick (`"tick: 0001\n"`…), then disables itself after N ticks.

**Device (tiny SP804-style):**

- Registers at, say, `0x0A000000`:
  - `LOAD +0x00`, `VALUE +0x04`, `CTRL +0x08` (bit0 enable, bit1 periodic, bit2 irq_en), `INTCLR +0x0C`, `RIS +0x10`, `MIS +0x14`.
- On `hw_bus_tick(cycles)`: decrement `VALUE`; if `VALUE==0`, set `RIS=1`; if `irq_en`, raise IRQ; if periodic, `VALUE=LOAD` after underflow. Clear on write to `INTCLR`.

**Handler (pseudo-asm):**

```
irq_handler:
    ldr   r4, =TIMER_BASE+INTCLR
    mov   r5, #1
    str   r5, [r4]              @ ack

    ldr   r1, =UART_BASE
    ldrb  r0, [r6], #1          @ r6 -> message "tick\n\0..."
    cmp   r0, #0
    beq   done
    str   r0, [r1, #0]          @ UART DR
    subs  pc, lr, #4

done:
    @ disable timer
    ldr   r2, =TIMER_BASE+CTRL
    mov   r3, #0
    str   r3, [r2]
    subs  pc, lr, #4
```

**Main:**

- Program timer: `LOAD = 100000`, `CTRL = enable|periodic|irq_en`.
- Enable IRQs (clear I bit), then loop `WFI`.

**How to test:**

- Run and watch UART output: `tick: 0001`, `tick: 0002`, … predictable cadence.

------

### Which one first?

- If you want the **fastest proof** of IRQ wiring: **UART RX echo**.
- If you want to validate **timing + IRQ** together: **timer tick**.

I can drop in ready-to-paste stubs (vector table + minimal asm for the handler and main) for either path, plus the tiny device code for the timer. Which one do you want to start with?