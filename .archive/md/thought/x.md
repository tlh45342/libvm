Does this look like the VM is executing the code faithfully?

###### ChatGPT said:

Thought for 2m 6s

Short version: it’s **stepping and halting**, but it’s **not executing the first 4 instructions faithfully**. Your log shows those four are being decoded/executed as `TST/CMP` instead of **`MOVW/MOVT`**, so `r0`/`r1` never get the intended non-zero values. That’s why later you see things like:

- `and r2, r0, r1` → uses `0 & 0`
- `and r4, r0, #0xFF000000` → yields `0`

Those would be non-zero if `MOVW/MOVT` had actually run.

### Why it happens

Your `K12_TABLE` puts the **DP (register) / DP (imm) rows before MOVW/MOVT**, and your per-key lists preserve that priority. So keys like `0x30F/0x34F` are being claimed by the TST/CMP entries before the MOVW/MOVT rules get a chance.

### Fix (2 small changes)

1. **Move MOVW/MOVT to the very top, before ANY DP rows** and keep the 32-bit disambiguator:

```
// ---- Wide moves (must precede any DP rows) ----
{ 0x0F00u, 0x0300u, 0x0FF00000u, 0x03000000u, true, handle_movw, "MOVW" },
{ 0x0F00u, 0x0340u, 0x0FF00000u, 0x03400000u, true, handle_movt, "MOVT" },

// ---- DP (register) class...
{ 0x0DE0u, 0x0000u, 0, 0, true, handle_and, "AND" },
...
```

1. **Make sure `execute()` uses the fast table**:

```
// in execute()
k12_ensure_built();
if (try_decode_key12_fast(instr)) return true;  // <-- use the fast one
```

…and remove any call to the old `try_decode_key12()` (and any leftover old linked-list code).

### Optional (but recommended)

- Update `disasm_line()` to print MOVW/MOVT (you have the snippet already). Then your first four lines will show `movw/movt` instead of `tst/cmp`.
- Quick sanity check after the fix: after 0x8000/0x8004, `r0` should be `0xF0F000F0`; after 0x8008/0x800C, `r1` should be `0x0F0F0F0F`.

Once you reorder those two entries and ensure the fast dispatcher is used, your VM will execute the code faithfully **and** the disasm will match `objdump`.

##### You said:

What K12 table... we should be useing th 4096 key table recomended.

###### ChatGPT said:

Thought for 52s

Yep—you **are** using the 4096-key fast table. When I said “K12 table,” I meant the small, ordered **seed rules array** you feed into the 4096 builder (your `K12_TABLE[]`). The 4096 map is *derived* from that array and **preserves its order per key**. So priority still matters.

Right now your log shows the first four instructions decoded/executed as `TST/CMP` instead of **MOVW/MOVT**. That happens because in your seed array the DP rows come **before** the MOVW/MOVT rows, so when the 4096 map is built, those DP candidates sit ahead of MOVW/MOVT for the same keys.

## Do this (2 tweaks)

1. **Reorder the seed rules** so MOVW/MOVT are above *all* DP rows:

```
// ---- Wide moves (must be first among data-processing style encodings) ----
{ 0x0F00u, 0x0300u, 0x0FF00000u, 0x03000000u, true, handle_movw, "MOVW" },
{ 0x0F00u, 0x0340u, 0x0FF00000u, 0x03400000u, true, handle_movt, "MOVT" },

// ---- DP (register) class: ...
{ 0x0DE0u, 0x0000u, 0, 0, true, handle_and, "AND" },
...
```

Then rebuild the fast table (your lazy `k12_ensure_built()` handles this on a fresh run; if you ever want to rebuild at runtime, expose a `k12_rebuild()` that clears `g_k12_ready` and calls the builder).

1. **Disassembler**: add the MOVW/MOVT check *early* in `disasm_line()` so your printed text matches objdump:

```
// at the top of disasm_line()
if ((instr & 0x0FF00000u) == 0x03000000u || (instr & 0x0FF00000u) == 0x03400000u) {
    uint32_t is_movt = ((instr & 0x0FF00000u) == 0x03400000u);
    uint32_t Rd   = (instr >> 12) & 0xF;
    uint32_t imm4 = (instr >> 16) & 0xF;
    uint32_t imm12=  instr & 0xFFF;
    uint32_t imm16= (imm4 << 12) | imm12;
    snprintf(out, out_sz, "%s r%u, #0x%X", is_movt ? "movt" : "movw", Rd, imm16);
    return;
}
```

## What you should see after the reorder

- `00008000` → `movw r0, #0xF0`
- `00008004` → `movt r0, #0xF0F0`
- `00008008` → `movw r1, #0xF0F`
- `0000800C` → `movt r1, #0xF0F`
- Later `and` ops will still compute to the same values except things like `r4 = r0 & 0xFF000000` will now be `0xF0000000` (it was `0` only because `r0` never got set).

Bottom line: the VM core is fine; it’s just decode priority. Put MOVW/MOVT first in the seed array that builds the 4096 map, rebuild, and your execution + disasm will line up with `objdump`.

##### You said:

?  Maybe I am missing something.  I thought the purpose was to reserve the 4096 table.  Pre insert everything... and order doesn't matter here as long as it get's completed.  And then we can call then O(1).  After that ORDER doesn't matter because we aren't using the old Table right?  In fact I am assuming you had me take it out as part of the clean up?  Or am I missing things?

###### ChatGPT said:

Thought for 40s

You’re not crazy—the whole point of the **4096-key table** is O(1) dispatch and less fragility. But there’s one subtlety:

- The 4096 table maps **one key → a small list of candidate rules** (because many rules use masks and can match the *same* key).
- If you **append** candidates in the order you scan your seed rules array, then the **first** candidate still “wins”.
   So even with the 4K map, **priority still exists** unless you explicitly sort those per-key candidate lists.

Right now your builder loops over `K12_TABLE[]` and pushes any matching rule into the per-key list. That means generic DP masks can sit in front of MOVW/MOVT if MOVW/MOVT appear later in the seed array—hence your `E300…/E34…` showing up as TST/CMP.

## Make order not matter (small patch)

During build, compute a **specificity score** per rule and keep each key’s candidate list **sorted by specificity (desc)**. Rules with an `xmask32` (exact 32-bit disambiguator) should always beat generic DP masks.

### 1) Add a tiny priority helper

```
static inline uint8_t popcnt16(uint16_t x) {
    x = (x & 0x5555u) + ((x >> 1) & 0x5555u);
    x = (x & 0x3333u) + ((x >> 2) & 0x3333u);
    x = (x + (x >> 4)) & 0x0F0Fu;
    x = x + (x >> 8);
    return (uint8_t)(x & 0x1Fu);
}

static inline uint8_t k12_priority(const k12_entry *e) {
    // Higher = more specific. Give a big bump to entries with xmask32.
    uint8_t p = popcnt16(e->mask12);
    if (e->xmask32) p += 32;   // force exact encodings (MOVW/MOVT, system) to front
    return p;
}
```

### 2) Store prios alongside indices and **insert** in order

Add a parallel priority array:

```
static uint16_t g_keylist[KEY12_SPACE][MAX_PER_KEY];
static uint8_t  g_keyprio[KEY12_SPACE][MAX_PER_KEY];
static uint8_t  g_keycount[KEY12_SPACE];
```

Change the builder to keep each key’s list sorted (descending):

```
static void k12_build_table(void) {
    for (int k = 0; k < KEY12_SPACE; ++k) g_keycount[k] = 0;

    const size_t N = sizeof(K12_TABLE)/sizeof(K12_TABLE[0]);
    for (int k = 0; k < KEY12_SPACE; ++k) {
        uint8_t cnt = 0;
        for (uint16_t ei = 0; ei < N; ++ei) {
            const k12_entry *e = &K12_TABLE[ei];
            if (((uint16_t)k & e->mask12) == e->value12) {
                uint8_t pr = k12_priority(e);

                // insert by priority (desc), stable on ties
                uint8_t pos = cnt;
                if (cnt < MAX_PER_KEY) {
                    while (pos > 0 && pr > g_keyprio[k][pos-1]) {
                        g_keyprio[k][pos] = g_keyprio[k][pos-1];
                        g_keylist[k][pos] = g_keylist[k][pos-1];
                        --pos;
                    }
                    g_keyprio[k][pos] = pr;
                    g_keylist[k][pos] = ei;
                    if (cnt < MAX_PER_KEY) ++cnt;
                } else {
                    // list full: maybe bump in if more specific than the last
                    if (pr > g_keyprio[k][cnt-1]) {
                        // insert then drop last
                        uint8_t p = cnt-1;
                        while (p > 0 && pr > g_keyprio[k][p-1]) {
                            g_keyprio[k][p] = g_keyprio[k][p-1];
                            g_keylist[k][p] = g_keylist[k][p-1];
                            --p;
                        }
                        g_keyprio[k][p] = pr;
                        g_keylist[k][p] = ei;
                    }
                }
            }
        }
        g_keycount[k] = cnt;
    }
    g_k12_ready = true;
}
```

Now **seed order truly doesn’t matter**: MOVW/MOVT (which have `xmask32`) will be automatically placed before generic DP matches for any shared key.

## Also fix your disasm text (optional but nice)

Your execution is already matching the right handlers once priority is proper, but the printed disasm lines for MOVW/MOVT still show `tst/cmp`. Put the MOVW/MOVT check **early** in `disasm_line()` so you print what you actually execute.

```
if ((instr & 0x0FF00000u) == 0x03000000u || (instr & 0x0FF00000u) == 0x03400000u) {
    uint32_t is_movt = ((instr & 0x0FF00000u) == 0x03400000u);
    uint32_t Rd   = (instr >> 12) & 0xF;
    uint32_t imm4 = (instr >> 16) & 0xF;
    uint32_t imm12=  instr & 0xFFF;
    uint32_t imm16= (imm4 << 12) | imm12;
    snprintf(out, out_sz, "%s r%u, #0x%X", is_movt ? "movt" : "movw", Rd, imm16);
    return;
}
```

## Sanity checks after this change

- After `0x8000/0x8004`: `r0 == 0xF0F000F0`
- After `0x8008/0x800C`: `r1 == 0x0F0F0F0F`
- Then `and r2, r0, r1` should produce `0x000000F0`, etc.
- Your log disasm lines will also say `movw/movt` rather than `tst/cmp`.

------

**Bottom line:** the 4K table is great, but without per-key **priority sorting** you still depend on the seed order. Add the tiny priority + insertion sort above, and you’ll get exactly what you expected: order-independent, O(1) dispatch with correct MOVW/MOVT beating generic DP every time.

##### You said:

Are you saying with Key12 we have collisions?  That one or more instruction may will match?

###### ChatGPT said:

Thought for 59s

Yep—**collisions are expected** with your `key12`.

Why: `key12 = [bits27..25 | bits24..20 | bits7..4]` only uses **12 bits** of the 32-bit word. Many encodings share those fields, and several of your rules intentionally use **loose masks** (e.g., ignore `S`, sometimes ignore `op3`) to cover whole instruction families. So **multiple rules can match the same key**. Examples:

- **MOVW/MOVT** live in the same “class” as data-processing immediates (op1 = `001`), so their `key12` overlaps with **TST/CMP (imm)** keys. Your MOVW/MOVT rows use `mask12=0x0F00` (checks only op1), so they’ll collide with many DP-imm entries by design.
- Load/store variants often share `key12` and are split by other bits (U/P/W/I), which your `key12` doesn’t include.

That’s why we add a second filter (`xmask32/xvalue32`) and a **per-key priority order**. With the 4096 map you still end up with “key → small candidate list”; you must decide which candidate wins:

- **Option A (easy):** keep seed array ordered by desired priority (exact/specific before generic).
- **Option B (order-free, recommended):** during the 4K build, **sort each key’s candidates by specificity**, e.g. popcount of `mask12` + a big bonus if `xmask32` is present. Then MOVW/MOVT (with `xmask32`) beat generic DP rules automatically.

Quick priority scorer (recap):

```
static inline uint8_t popcnt16(uint16_t x){ x=(x&0x5555)+((x>>1)&0x5555);
  x=(x&0x3333)+((x>>2)&0x3333); x=(x+(x>>4))&0x0F0F; x=x+(x>>8); return x&0x1F; }

static inline uint8_t k12_priority(const k12_entry *e) {
  uint8_t p = popcnt16(e->mask12);
  if (e->xmask32) p += 32; // exact encodings first
  return p;
}
```

Then insert each match into the per-key list **sorted by descending priority**. After that, seed order truly doesn’t matter, and collisions are resolved deterministically.

If you want to see collisions in action, add a tiny debug helper:

```
static void dump_key_candidates(uint32_t instr){
  uint16_t k = key12(instr);
  uint8_t cnt = g_keycount[k];
  log_printf("[K12] candidates for 0x%03X:", k);
  for (uint8_t i=0;i<cnt;i++){
    const k12_entry *e = &K12_TABLE[g_keylist[k][i]];
    log_printf(" %s", e->name);
  }
  log_printf("\n");
}
```

Call it under `DBG_K12` for a problematic instruction (e.g., `E30000F0`); you’ll see both MOVW and TST show up—the priority/sort decides which executes.

##### You said:

At the end the right instruction executes right?

###### ChatGPT said:

Thought for 29s

Right now, **no**—your own log shows the *wrong* handlers firing at 0x8000..0x800C:

- `[K12] TST match` at 0x8000
- `[K12] CMP match` at 0x8004
- …instead of **MOVW/MOVT**.

That’s why `r0`/`r1` stay zero and all the later ANDs are with 0.

This happens because multiple rules hit the same **key12** and your 4K map keeps candidates in the order you fed them. Since DP rules appear before MOVW/MOVT in your seed list, they win.

## Fix (pick one)

1. **Quickest:** move the two MOVW/MOVT rows to the **top** of your seed rules (above all DP entries), rebuild, and run again. You should then see:
   - `[K12] MOVW match` at 0x8000
   - `[K12] MOVT match` at 0x8004
2. **Order-independent (recommended):** in the 4K builder, **sort each key’s candidates by specificity**, e.g. popcount(mask12) + big bonus if `xmask32` is set. Then MOVW/MOVT automatically beat generic DP rules even if the seed list is shuffled.

Also make sure `execute()` calls your **fast** dispatcher (`try_decode_key12_fast`) and not any old linear version.

## What to expect after the fix

- After 0x8000/0x8004: `r0 == 0xF0F000F0`
- After 0x8008/0x800C: `r1 == 0x0F0F0F0F`
- Log lines at those PCs show `MOVW/MOVT` matches, not TST/CMP.
- Your AND results change accordingly (e.g., `r2 = r0 & r1` becomes nonzero).

If you want to verify collisions explicitly, dump the per-key candidate list for the key at 0x8000—you’ll see both MOVW and TST there; after the fix, MOVW will be listed first and will execute.

##### You said:

But what happens if we execute TST ?

###### ChatGPT said:

Thought for 29s

Short answer: **TST only updates flags—no registers.**

On ARM, `tst Rn, Operand2` computes `Rn & Operand2`, writes **no destination register**, and sets flags:

- **Z** = 1 if the AND is 0, else 0
- **N** = bit 31 of the AND
- **C** = shifter carry-out from `Operand2` (for an immediate with rotate=0, C is usually left unchanged)
- **V** is **not** affected

In your run:

- At `0x8000` the VM decoded `E30000F0` as `tst r0, #0xF0`. With `r0=0`, result is 0 ⇒ **Z=1**, `r0` stays **0**.
- At `0x801C` you have `tst r0, #0x1` again ⇒ still **Z=1** since `r0` is 0.
- At `0x8040` `tst r9, r9` with `r9=0` ⇒ **Z=1**.
- Then `0x8044` is `ANDNE …` (cond = NE, i.e., execute if Z==0), but **Z==1**, so you see `[AND cond fail (0x1)]` and it’s skipped.

Because those early MOVW/MOVT didn’t run (they were mis-decoded as TST/CMP), `r0`/`r1` never get loaded, so all later ANDs with them stay 0. You still hit `DEADBEEF` and halt, but the **data path is wrong**. Fixing the decode priority (or sorting candidates by specificity in the 4K map) will make the first four instructions execute as MOVW/MOVT, load the constants, and produce the expected nonzero results.

##### You said:

Will TST execute as TST ?

###### ChatGPT said:

Thought for 47s

Yes—once you fix candidate priority (either by putting MOVW/MOVT before DP in the seed list or by sorting each key’s candidates by “specificity”), a real **TST** encoding will still execute as **TST**.

Why it’s safe:

- Your MOVW/MOVT rules have an extra check: `xmask32 = 0x0FF00000`.
  - MOVW requires bits27..20 = **0x30** (i.e., `0x03000000`).
  - MOVT requires bits27..20 = **0x34** (i.e., `0x03400000`).
- **TST (imm)** has bits27..20 = **0x31** (I=1, opcode=1000, S=1) → `0x03100000`.
- So even if MOVW/MOVT are tried first, they **won’t match** a TST word (their `xmask32` equality fails). The dispatcher then tries the next candidate, which is TST, and **TST runs**.

Bottom line: with proper per-key ordering (specific before generic), MOVW/MOVT won’t “steal” a TST, and a genuine TST will execute as TST (update flags only, no register write).

##### You said:

So the xmask32 is not part of key12 but an extra bit of stuffing we tack on to seperate the MOVs from the STS?

###### ChatGPT said:

Thought for 11s

Exactly. key12 is just a **coarse index** (12 bits: 27–25, 24–20, 7–4) to drop us into an O(1) bucket. Multiple encodings collide there on purpose.

`xmask32/xvalue32` is an **extra 32-bit predicate** you apply *after* the key12 bucket match to disambiguate look-alikes. It lets you check any bits in the full word—most importantly bits 27..20—so you can separate MOVW/MOVT from TST/CMP immediates that share the same key12.

Quick examples (values are bits 27..20):

- **MOVW**: `0x03000000` → we use `xmask32=0x0FF00000, xvalue32=0x03000000`
- **MOVT**: `0x03400000` → `xmask32=0x0FF00000, xvalue32=0x03400000`
- **TST (imm)**: `0x03100000` → fails the MOVW/MOVT xmask32 check, passes TST
- **CMP (imm)**: `0x03500000` → fails MOVW/MOVT, matches CMP

So: bucket with key12 → iterate candidates → check `xmask32/xvalue32` (and cond) → first pass wins. The `xmask32` is optional (0 means “no extra check”), and we use it for exact patterns (NOP/WFI/DSB/ISB, DEADBEEF) and to keep MOVW/MOVT from being mistaken for TST/CMP.

(And yep, I assume you meant **TST**, not “STS.”)

##### You said:

What genius came up with KEY12 and decided to stop there?  my guess is that this based on experimental data that people have tested over time and feel this is a good short term generally accepted algorithm factoring in smaller space for with extra work in lieu of more space which was empty?

###### ChatGPT said:

Thought for 10s

haha—no shadowy standards committee here 😊. “KEY12” is just a **pragmatic trick** decoder writers use: pick a small set of **high-discriminating bitfields**, build a 2ⁿ bucket table from them, and finish with a couple of exact checks. It isn’t an official ARM thing; it’s “what works well” for a fast, compact software decoder.

Why these 12 bits (27–25, 24–20, 7–4)?

- **They split the ISA nicely.**
  - 27–25 (“op1”) separates big classes (DP, L/S, branch…).
  - 24–20 (“op2/opcode+S”) gets you down to families (AND/ORR/TST/CMP…).
  - 7–4 (“op3”) helps distinguish addressing/shift subforms.
- **Small & cache-friendly.** 4096 buckets is tiny: easy to precompute, fits in L1, O(1) dispatch.
- **Low collision rate** in practice. Most buckets only have a handful of candidates. For the few that collide (like MOVW/MOVT vs TST/CMP), you add a **secondary exact predicate** (`xmask32/xvalue32`) and (optionally) a **priority** rule. Done.

Could we go bigger than 12? Sure:

- **16–20 bits** → fewer collisions, but the table explodes (e.g., 2²⁰ = 1,048,576 entries). Most of those would be empty, i.e., you trade RAM for sparse nothingness.
- **Smaller than 12** → more collisions → more work after the bucket.

So KEY12 is the “nice elbow” on the curve: *very* low memory, still O(1), and just a few follow-up checks. That’s why you see this pattern (or a close cousin) show up in a lot of emulators and JITs: a **coarse index** → **tiny candidate list** → **exact match & cond check**.

TL;DR: it’s not gospel, it’s **engineering ergonomics**—small, fast, and good enough that the rare collisions (like your MOVW/TST) are trivial to break with one extra mask and a little per-key priority.

##### You said:

has any one done a rigorous study to show what key12 of 13 produces the fewest collisions. i.e.  To discover if there might be a case where picking say (every other bit) might reduce this to O(1) with no collisions?

###### ChatGPT said:

Thought for 36s

Short answer: I don’t know of a published, rigorous “pick-the-best 12/13 bits” study for ARM/AArch32. What people do in practice is either:

- auto-generate a **decision tree / trie** from the ISA spec (LLVM TableGen’s decoder generator, CGEN, GLISS, etc.), or
- use a small coarse key (like your key12) as an O(1) bucket, then run a couple **exact predicates** (`xmask32`) inside the bucket.

That first approach makes the “which bits?” question moot because the generator chooses the tests that minimize collisions or decode depth automatically. [LLVM](https://llvm.org/docs/TableGen/ProgRef.html?utm_source=chatgpt.com)[reviews.llvm.org](https://reviews.llvm.org/D120958?utm_source=chatgpt.com)[Pipat Methavanitpong](https://fulcronz27.wordpress.com/2014/12/24/using-cgen-to-generate-files-for-binutils/?utm_source=chatgpt.com)[ResearchGate](https://www.researchgate.net/publication/224584688_A_versatile_generator_of_instruction_set_simulators_and_disassemblers?utm_source=chatgpt.com)

### Could there be a magic 12–13-bit subset with zero collisions?

For AArch32, effectively **no** in the general case. Many encodings deliberately share high-level fields, and only differ on bits you *didn’t* include in key12. Unless your subset contains **all** discriminating bits for **every** pair of encodings, collisions are inevitable. That’s why practical decoders add a second-stage exact check (your `xmask32`) or use a generated decision tree.

### If you want to *search* for a “best” key

You can do it empirically over your own rule set:

1. Take all `(mask, value)` rules.
2. Define a candidate bit set `S`. Two rules “collide” under `S` if they don’t differ on any bit from `S` that both care about.
3. Score `S` by total collisions (or max bucket size).
4. Pick `S` via a greedy “information gain” heuristic (add the bit that reduces collisions the most each step).
5. Stop at 12/13 bits.

This is essentially a **set-cover / decision-tree** problem; generators in the literature build near-optimal trees automatically rather than fixing a single 12-bit hash. [ACM Digital Library+1](https://dl.acm.org/doi/pdf/10.5555/2561828.2561972?utm_source=chatgpt.com)

### If you really want O(1) with no collisions

You can compute a **perfect hash** over the *finite set of valid encodings* (or opcode IDs) and jump directly to handlers. This is common in other domains; modern minimal-perfect hashing is fast and compact, but it’s typically over enumerated keys, not raw 32-bit words with don’t-cares. You’d still need a pre-normalization step that maps a 32-bit instruction to a unique “encoding ID”. [arXiv](https://arxiv.org/pdf/2506.06536?utm_source=chatgpt.com)[CMU School of Computer Science](https://www.cs.cmu.edu/~mheule/publications/AAAI-WeaverS.1625.pdf?utm_source=chatgpt.com)

**Bottom line:** there isn’t a canonical “best 12/13 bits” for ARM in the literature. The standard, scalable solutions are (a) **generated decision trees** from the ISA spec, or (b) your current **coarse key + exact predicates**, optionally with per-bucket priority/specificity sorting to make the result seed-order-independent.