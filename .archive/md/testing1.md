This is my directory of test-cases: 8/13/2025  10:53 PM    <DIR>          . 08/13/2025  10:53 PM    <DIR>          .. 07/31/2025  03:42 PM    <DIR>          disk_read_test 08/03/2025  10:21 PM    <DIR>          hello_world 07/29/2025  10:46 AM    <DIR>          min-kernel1 08/10/2025  10:09 PM    <DIR>          test_add 08/10/2025  10:45 PM    <DIR>          test_and 08/04/2025  03:50 PM    <DIR>          test_blbx 08/13/2025  10:57 PM    <DIR>          test_branch 07/28/2025  05:15 PM    <DIR>          test_bxlr 07/29/2025  12:59 PM    <DIR>          test_cross 07/28/2025  03:22 PM    <DIR>          test_crt 08/04/2025  04:46 PM    <DIR>          test_disk_read 08/03/2025  10:51 AM    <DIR>          test_ldr 07/28/2025  05:43 PM    <DIR>          test_ldrb 08/03/2025  11:17 AM    <DIR>          test_putchar 08/03/2025  10:36 AM    <DIR>          test_putchars 07/28/2025  03:12 PM    <DIR>          test_stack 07/28/2025  07:50 PM    <DIR>          test_stack_ballr 08/08/2025  10:52 AM    <DIR>          test_svc 08/10/2025  10:19 PM    <DIR>          test_wfi Let me know what you think.  I think it might be a good idea to rename these things with numbers so that they can be evaluated in some kind of order.  i.e. simpler tests followed by harder tests.  This was the simpler tests can prove foundation elements for testing more complex issues.  Thoughts?  And if so can you recomend an ordering?

###### ChatGPT said:

Thought for 8s

Love this idea. A numbered, layered test suite makes debugging way saner. Here’s a clean, incremental order and a rename plan that goes from “CPU basics” → “loads/stores” → “control flow” → “stack & calls” → “I/O & system” → “MMIO/disks” → “integration”.

# Recommended order + new names

01–09: Core ALU & loads/stores
 10–19: Branching & subroutines
 20–29: Stack & multi-register
 30–39: Console/UART/CRT
 40–49: System (WFI/SVC)
 50–69: MMIO & disk
 70–99: Integration / misc

Proposed mapping (left = current dir, right = new dir):

- 01_test_add            ← `test_add` (ADD/MVN basics)
- 02_test_and            ← `test_and`
- 03_test_ldr_literal    ← `test_ldr` (if it’s literal/PC+8 focused)
- 04_test_ldrb           ← `test_ldrb`
- 10_test_branch         ← `test_branch` (conditional branches sanity)
- 11_test_blbx           ← `test_blbx`   (BL/B, conds honored)
- 12_test_bxlr           ← `test_bxlr`   (return via LR/BX)
- 20_test_stack_basic    ← `test_stack`  (STM/LDM/POP/PUSH basics)
- 21_test_stack_ballr    ← `test_stack_ballr` (stack + BL/ALU/ LR interplay)
- 30_test_putchar        ← `test_putchar`  (UART write path)
- 31_test_putchars       ← `test_putchars` (looped LDRB + UART/CRT)
- 32_test_crt            ← `test_crt`      (full-screen text buffer)
- 40_test_wfi            ← `test_wfi`
- 41_test_svc            ← `test_svc`
- 50_test_disk_read      ← `test_disk_read` (single-sector via MMIO)
- 51_disk_read_test      ← `disk_read_test` (keep if it differs; otherwise merge into 50)
- 70_min_kernel1         ← `min-kernel1`   (integration smoke)
- 80_test_cross          ← `test_cross`    (ASM↔C interop; integration)

If `hello_world` is just a hello on UART, slot it at:

- 00_hello_world         ← `hello_world` (pure smoke test, zero dependencies)

If `test_ldr` is broader than PC-literal, you can split later into:

- 03_test_ldr_literal
- 05_test_ldr_prepost

# Why this flow works

- **00–04** prove register ops and memory fetches (incl. PC+8 semantics) before control flow.
- **10–12** lock down condition flags/branches/links/returns.
- **20–21** validates stack discipline and multi-reg transfers before deeper calls.
- **30–32** exercises output paths (UART/CRT) only after basic CPU is solid.
- **40–41** adds architectural “system-ish” bits.
- **50–51** touches MMIO & disk once memory and control flow are stable.
- **70–80** run broader, multi-feature scenarios.

# Practical renaming (Windows cmd)

From `test-cases\`:

```
ren test_add              01_test_add
ren test_and              02_test_and
ren test_ldr              03_test_ldr_literal
ren test_ldrb             04_test_ldrb

ren test_branch           10_test_branch
ren test_blbx             11_test_blbx
ren test_bxlr             12_test_bxlr

ren test_stack            20_test_stack_basic
ren test_stack_ballr      21_test_stack_ballr

ren test_putchar          30_test_putchar
ren test_putchars         31_test_putchars
ren test_crt              32_test_crt

ren test_wfi              40_test_wfi
ren test_svc              41_test_svc

ren test_disk_read        50_test_disk_read
rem If different, keep; else delete after merging:
rem ren disk_read_test    51_disk_read_test

ren min-kernel1           70_min_kernel1
ren test_cross            80_test_cross

ren hello_world           00_hello_world
```

(# If any directory doesn’t exist in your tree, skip its `ren`.)

# Make/test runner tip

Have `run_tests.py` just glob directories and sort them lexicographically; the numeric prefixes will naturally enforce order. Pseudocode:

```
import os

roots = [d for d in os.listdir('.') if os.path.isdir(d) and d[0].isdigit()]
roots.sort()  # "00_" .. "80_" order
for d in roots:
    # run your existing per-test harness in d
```

If you want to keep backward compatibility for a while, you can let the runner include both numbered and legacy names, but sort numbered first.

# Bonus: tag each test with a tiny README

Drop a 4–6 line `README.md` in each folder listing:

- **Goal** (one sentence)
- **Instructions covered** (ADD/MVN, LDR literal, etc.)
- **Expected side effects** (UART, CRT, disk)
- **Key CHECKS** strings

That makes failures self-explanatory when you come back later.

If you’d like, I can spit out a quick `run_tests.py` that sorts by prefix and prints a compact summary like your current output.