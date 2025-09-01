I think we are ok for use of TAP.  We will dive into that a bit later... but I am looking forward to it.  How does the CRT work?  We had been using just the fixed memory addresses.  Do we change that?  What does that look like?



###### ChatGPT said:

Thought for 23 seconds

Great question. We can keep your **existing fixed-address text buffer working exactly as-is**, and add a tiny **MMIO control block** so we can (a) query/set geometry, (b) switch between **text** and **bitmap** modes, (c) enable **double‑buffering**, and (d) signal the host (“WINCRT”) to repaint efficiently.

Think of it as **backward‑compatible + optional controls**.

------

## TL;DR — What changes for you?

- **Nothing has to change** if you want to keep writing to the old text buffer address.
- You **can** start using a few MMIO registers at `0x4001_0000` (suggested base) to:
  - confirm rows/cols, configure the **text buffer base pointer** (we’ll default it to your legacy address),
  - switch to a **bitmap framebuffer** later,
  - request **screen updates** (dirty rects or full flush),
  - use **double buffering** without changing your drawing code much.

------

## Device model (guest view)

**Base**: `CRT0` at `0x4001_0000` (fits our MMIO map from earlier).

### Registers (little‑endian, 32‑bit unless noted)

```
diffCopyEdit+0x000 ID        (R)  0x43525430  // "CRT0"
+0x004 VERSION   (R)  0x0001_0000 // v1.0
+0x008 CAPS      (R)  bit0=TEXT, bit1=BITMAP, bit2=DOUBLEBUF, bit3=PALETTE, bit4=CURSOR_HW

// --- Common control ---
+0x00C MODE      (R/W) 0=OFF, 1=TEXT, 2=BITMAP
+0x010 IRQ_EN    (R/W) bit0=VSYNC, bit1=DIRTY_DONE
+0x014 STATUS    (R/W1C) bit0=VSYNC_PEND, bit1=DIRTY_ACK

// --- Text mode (legacy-friendly) ---
+0x020 T_COLS    (R/W) default 80
+0x024 T_ROWS    (R/W) default 25
+0x028 T_CELL_SZ (R)   bytes per cell (default 2: char,attr) — see formats below
+0x02C T_BASE    (R/W) guest-physical address of text buffer
+0x030 T_CUR_X   (R/W)
+0x034 T_CUR_Y   (R/W)
+0x038 T_ATTR    (R/W) default attribute (fg/bg)
+0x03C T_FLAGS   (R/W) bit0=CURSOR_EN, bit1=BLINK

// --- Bitmap mode (framebuffer) ---
+0x040 B_WIDTH   (R/W) e.g., 640
+0x044 B_HEIGHT  (R/W) e.g., 480
+0x048 B_BPP     (R/W) 32 (ARGB8888) or 8 (indexed)
+0x04C B_PITCH   (R)   bytes per line (VM fills after BPP/width set)
+0x050 B_FB0     (R/W) primary framebuffer address (guest RAM)
+0x054 B_FB1     (R/W) back buffer (if DOUBLEBUF)
+0x058 B_ACTIVE  (R/W) 0=FB0, 1=FB1 (flip on write)
+0x05C B_PALETTE (R/W) palette base if BPP=8

// --- Dirty rectangle / flush control (optional but efficient) ---
+0x060 UPD_X     (R/W)
+0x064 UPD_Y     (R/W)
+0x068 UPD_W     (R/W)
+0x06C UPD_H     (R/W)
+0x070 UPD_KICK  (W)   write 1 to request redraw of region
```

> **Compatibility:** On reset, the device sets `MODE=TEXT`, `T_COLS=80`, `T_ROWS=25`, `T_BASE=<legacy_address>`. So your old code “just works.” You can migrate at your pace.

### Text buffer formats

Pick one and we’ll lock it in the device:

- **Simple (default)**: **2 bytes per cell**
   `byte0 = ASCII`, `byte1 = attribute` (4‑bit FG, 4‑bit BG like PC text).
   This matches what many hobby CRTs do and is easy to keep.
- (Optional) **1 byte per cell** (ASCII only; global attr in `T_ATTR`). If you prefer this, we can expose `T_CELL_SZ=1` and the host will render using `T_ATTR`.

------

## How the host WINCRT side uses this

- When you write **text cells** or a **bitmap**, the guest pokes `UPD_*` and writes `UPD_KICK=1`.
   The emulator copies the data from **guest RAM** at `T_BASE` or `B_FB*` and **repaints** the window.
- If you enable **DOUBLEBUF** and flip `B_ACTIVE`, the host swaps which buffer it reads from next refresh.
- If you want **VSYNC IRQ**, enable `IRQ_EN.bit0`; the emulator will assert it at a fixed refresh rate (e.g., 60 Hz) so you can time animations without busy‑waiting.

------

## Minimal guest driver (text mode, legacy‑friendly)

```
cCopyEdit// crt.h
#pragma once
#include <stdint.h>

#define CRT_BASE      0x40010000u
#define REG32(off)    (*(volatile uint32_t*)(CRT_BASE + (off)))

#define CRT_ID        REG32(0x000)
#define CRT_VERSION   REG32(0x004)
#define CRT_MODE      REG32(0x00C)
#define CRT_T_COLS    REG32(0x020)
#define CRT_T_ROWS    REG32(0x024)
#define CRT_T_CELL_SZ REG32(0x028)
#define CRT_T_BASE    REG32(0x02C)
#define CRT_T_CUR_X   REG32(0x030)
#define CRT_T_CUR_Y   REG32(0x034)
#define CRT_T_ATTR    REG32(0x038)
#define CRT_UPD_X     REG32(0x060)
#define CRT_UPD_Y     REG32(0x064)
#define CRT_UPD_W     REG32(0x068)
#define CRT_UPD_H     REG32(0x06C)
#define CRT_UPD_KICK  REG32(0x070)

#define CRT_MODE_OFF   0
#define CRT_MODE_TEXT  1
#define CRT_MODE_BMAP  2

// Choose a legacy base you already used; we can keep it the same.
#define TEXT_BASE_ADDR 0x00100000u  // example legacy address in RAM

static inline void crt_text_init(uint32_t cols, uint32_t rows) {
    if (CRT_ID != 0x43525430) return; // "CRT0"
    CRT_MODE = CRT_MODE_TEXT;
    CRT_T_COLS = cols;
    CRT_T_ROWS = rows;
    CRT_T_BASE = TEXT_BASE_ADDR;  // keep your old buffer
    CRT_T_ATTR = 0x07;            // light grey on black
}

static inline void crt_text_flush_region(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    CRT_UPD_X = x; CRT_UPD_Y = y; CRT_UPD_W = w; CRT_UPD_H = h;
    CRT_UPD_KICK = 1;
}
```

**Write characters exactly the way you used to** (into `TEXT_BASE_ADDR`). When you’re done with a line or the whole screen, call `crt_text_flush_region(0,0,cols,rows);` to repaint. If you forget to flush, the emulator can still do periodic refreshes (slower).

------

## Minimal guest driver (bitmap mode, later)

```
cCopyEdit#define CRT_B_WIDTH   REG32(0x040)
#define CRT_B_HEIGHT  REG32(0x044)
#define CRT_B_BPP     REG32(0x048)
#define CRT_B_PITCH   REG32(0x04C)
#define CRT_B_FB0     REG32(0x050)
#define CRT_B_FB1     REG32(0x054)
#define CRT_B_ACTIVE  REG32(0x058)

#define FB0_ADDR 0x00200000u
#define FB1_ADDR 0x00400000u

static inline void crt_bitmap_init(uint32_t w, uint32_t h) {
    CRT_B_WIDTH = w; CRT_B_HEIGHT = h; CRT_B_BPP = 32; // ARGB8888
    CRT_B_FB0 = FB0_ADDR;
    CRT_B_FB1 = FB1_ADDR;
    CRT_MODE = CRT_MODE_BMAP;
    // VM fills PITCH; you can read CRT_B_PITCH if needed
}
```

Draw into `FB0_ADDR`, call `crt_text_flush_region(x,y,w,h)` (same UPD registers), or flip to `FB1` for double‑buffered animation (`CRT_B_ACTIVE=1`, draw next frame in the other).

------

## Migration path

1. **Today**: keep writing your legacy text buffer at the same RAM address. Add **one line** in early init:
    `CRT_T_BASE = <legacy_address>; CRT_MODE = TEXT;` (or just rely on reset defaults).
2. Add **optional** flush calls so the host can repaint immediately after you draw.
3. When you want features (cursor, colors, resize, bitmap), start reading/writing the CRT registers—no need to change the rest of your code.

------

## What I need from you

- The **current fixed addresses** and **layout** you used for the legacy text buffer (address, rows/cols, 1‑byte vs 2‑byte cells, attribute bits). I’ll set those as **device reset defaults** so *zero* guest changes are required.
- If you already have a “clear screen”/“putc” helper, point me to it; I’ll wire it to the flush call so WINCRT updates instantly.

If the above looks good, I’ll:

- Drop a `crt.h` with these offsets,
- Provide a tiny guest helper (`crt_text_puts`, `crt_text_clear`, `crt_flush`),
- Add the **WINCRT device** in the emulator that watches `T_BASE`/`UPD_*` and renders your buffer exactly like before—just faster and more flexible.