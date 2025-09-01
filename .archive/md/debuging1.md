### rom your `kernel.bin` hex dump:

```
rCopyEdit0000000 d01c e59f  <- ldr sp, [pc, #28]
0000004 e01c e59f  <- ldr lr, [pc, #28]
0000008 e004 e52d  <- push {lr}
000000c 0092 eb00  <- bl 0x825c ✔️ this is correct
```

Decoded:

- `0xe59fd01c` = `ldr sp, [pc, #0x1c]` → loads from `0x802c`, i.e., value at offset `0x802c + 0x1c = 0x8048`, which contains `0x1FFFFFFC` (stack)
- `0xe59fe01c` = `ldr lr, [pc, #0x1c]` → value: `0x8010` (`halt_loop`)
- `0xe52de004` = `push {lr}`
- `0xeb000092` = `bl 0x825c` = `0x800c + (0x92 << 2)` = `0x800c + 0x248` = `0x8254` → wait, let's double check that...

Wait...

```
yamlCopyEdit0x800c + 4 = 0x8010
0x825c - 0x8010 = 0x24c
0x24c / 4 = 0x93  → so to reach 0x825c, you need bl immediate = 0x93
But your hex dump shows:
    000000c: 0092 eb00
```

Little-endian decode:

- `eb00 0092` = `0xeb000092`
- 0x92 = 146
- 146 * 4 = 584
- 0x800c + 584 = **0x8254**

Wait — this is **off by 8 bytes**! So **you are actually branching to 0x8254**, not `0x825c`.