question.  If I was going to write something for baremetal Raspberry PI 3.   What would I need to write to do something as simple as write hello world to the display? and then loop or wfi?



###### ChatGPT said:

Thought for 1m 1s

Totally doable! On a Raspberry Pi 3 you can get “Hello, world!” on the HDMI display with a tiny bare-metal kernel that:

1. boots in either AArch32 (`kernel.img`) or AArch64 (`kernel8.img`),
2. asks the GPU (via the **mailbox property interface**, channel 8) to create a **framebuffer**,
3. writes pixels into that framebuffer to draw text, then
4. spins in a `wfi` loop.

Below is a compact, end-to-end plan (plus minimal code you can drop into a project). I’ll default to **AArch64** since Pi 3 handles that cleanly with `kernel8.img`; if you prefer 32-bit I’ll note the tweak.

# Files on the SD card (FAT partition)

- Your kernel: **`kernel8.img`** (for AArch64) — the firmware will boot 64-bit with this name. Alternatively, use `kernel.img` for 32-bit. [S Matyukevich](https://s-matyukevich.github.io/raspberry-pi-os/docs/lesson01/rpi-os.html?utm_source=chatgpt.com)[Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=187136&utm_source=chatgpt.com)

- Minimal `config.txt` (optional but nice for UART):

  ```
  iniCopyEditenable_uart=1
  # If you named your kernel something else:
  # kernel=kernel8.img
  ```

- Usual boot firmware files (`start.elf`, etc.), as you do for other bare-metal tutorials. [Barely Coding](https://adamransom.github.io/posts/raspberry-pi-bare-metal-part-1.html?utm_source=chatgpt.com)

# Linker script (AArch64)

Link your image to the usual Pi address:

```
ldCopyEdit/* link.ld */
ENTRY(_start)
SECTIONS {
  . = 0x80000;            /* Pi 3 kernel entry for 64-bit kernels */
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }
}
```

(Use `. = 0x8000` for many 32-bit tutorials.) [OSDev Wiki](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones?utm_source=chatgpt.com)[Raspberry Pi Stack Exchange](https://raspberrypi.stackexchange.com/questions/26442/does-the-kernel-img-file-loaded-at-physical-0x00000000-or-0x00008000?utm_source=chatgpt.com)

# Startup stub (AArch64)

```
asmCopyEdit// start.S
.global _start
_start:
    // simple stack at end of BSS (linker provides symbol)
    ldr   x0, =_stack_top
    mov   sp, x0
    bl    kernel_main

1:  wfi
    b     1b

// Let the linker place _stack_top at end of BSS:
```

# Mailbox + framebuffer init (C)

The Pi 3 (BCM2837) peripheral base is **0x3F000000**. The **mailbox registers** are at base+0xB880. We’ll use the **property channel (8)** to request a 32-bit framebuffer and get its address and pitch. (Buffer must be 16-byte aligned; property messages stuff multiple “tags” like set size, set depth, alloc buffer, get pitch.) [Raspberry Pi 4 Operating System](https://www.rpi4os.com/part5-framebuffer/)[Raspberry Pi OS](https://jsandler18.github.io/extra/prop-channel.html?utm_source=chatgpt.com)

```
cCopyEdit// fb.c
#include <stdint.h>

#define PERIPHERAL_BASE  0x3F000000u             // Pi 2/3 base
#define MBOX_BASE        (PERIPHERAL_BASE + 0xB880)
#define MBOX_READ        (MBOX_BASE + 0x00)
#define MBOX_STATUS      (MBOX_BASE + 0x18)
#define MBOX_WRITE       (MBOX_BASE + 0x20)
#define MBOX_FULL        0x80000000u
#define MBOX_EMPTY       0x40000000u
#define MBOX_CH_PROP     8

static inline void mmio_write(uint32_t r, uint32_t v){ *(volatile uint32_t*)r = v; }
static inline uint32_t mmio_read(uint32_t r){ return *(volatile uint32_t*)r; }

// 16-byte aligned as only upper 28 bits are sent through the mailbox
static volatile uint32_t __attribute__((aligned(16))) mbox[36];

static int mbox_call(unsigned ch) {
    uint32_t addr = ((uint32_t)(uintptr_t)&mbox & ~0xF) | (ch & 0xF);
    while (mmio_read(MBOX_STATUS) & MBOX_FULL) {}     // wait to write
    mmio_write(MBOX_WRITE, addr);
    for(;;) {
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY) {} // wait for reply
        if (mmio_read(MBOX_READ) == addr)              // ours?
            return mbox[1] == 0x80000000;              // response OK?
    }
}

static uint8_t* fb;
static uint32_t pitch, fb_w, fb_h;

int fb_init(unsigned w, unsigned h) {
    // Build a property buffer: set phys/virt size, depth=32, alloc fb, get pitch
    mbox[0]  = 35*4;          // size in bytes
    mbox[1]  = 0;             // request

    mbox[2]  = 0x00048003;    // set physical width/height
    mbox[3]  = 8; mbox[4] = 8;
    mbox[5]  = w; mbox[6] = h;

    mbox[7]  = 0x00048004;    // set virtual width/height
    mbox[8]  = 8; mbox[9] = 8;
    mbox[10] = w; mbox[11]= h;

    mbox[12] = 0x00048005;    // set depth
    mbox[13] = 4; mbox[14]=4;
    mbox[15] = 32;

    mbox[16] = 0x00040001;    // allocate framebuffer
    mbox[17] = 8; mbox[18]=8;
    mbox[19] = 4096;          // alignment
    mbox[20] = 0;

    mbox[21] = 0x00040008;    // get pitch
    mbox[22] = 4; mbox[23]=4;
    mbox[24] = 0;

    mbox[25] = 0;             // end

    if (!mbox_call(MBOX_CH_PROP)) return -1;

    // Framebuffer address is a GPU bus address: clear top bits to get ARM physical
    // (common practice on Pi is to mask to 0x3FFFFFFF)
    uint32_t fb_bus = mbox[20] ? mbox[20] : mbox[19];
    fb     = (uint8_t*)(uintptr_t)(fb_bus & 0x3FFFFFFFu);
    pitch  = mbox[24];
    fb_w   = w; fb_h = h;
    return fb ? 0 : -1;
}
```

(Alloc/pitch/depth tag IDs and mailbox details: property channel 8, set depth tag `0x00048005`, get pitch `0x00040008`, allocate framebuffer `0x00040001`, etc.) [Scribd](https://www.scribd.com/document/669566109/Mailbox-property-interface-Message-TAGS-1?utm_source=chatgpt.com)[GitHub](https://github.com/LdB-ECM/Raspberry-Pi/blob/master/10_virtualmemory/rpi-SmartStart.h?utm_source=chatgpt.com)[Raspberry Pi 4 Operating System](https://www.rpi4os.com/part5-framebuffer/)

# Draw a tiny string

You need a bitmap font (e.g., 8×8). Easiest: include a tiny 8×8 public-domain font table and blit. Here’s the minimal drawing glue (assumes you provide `font8x8_basic[128][8]`):

```
cCopyEditextern const unsigned char font8x8_basic[128][8];

static inline void putpixel(int x,int y, uint32_t argb){
    *(uint32_t*)(fb + y*pitch + x*4) = argb;
}

static void draw_char(int x, int y, char c, uint32_t fg, uint32_t bg){
    const unsigned char *g = font8x8_basic[(unsigned)c];
    for(int row=0; row<8; ++row){
        unsigned char bits = g[row];
        for(int col=0; col<8; ++col){
            putpixel(x+col, y+row, (bits & (1<<col)) ? fg : bg);
        }
    }
}

static void draw_str(int x, int y, const char* s, uint32_t fg, uint32_t bg){
    for(int i=0; s[i]; ++i) draw_char(x + i*8, y, s[i], fg, bg);
}
```

# Your `kernel_main`

```
cCopyEdit// main.c
extern int fb_init(unsigned w, unsigned h);
extern void draw_str(int x,int y, const char* s, uint32_t fg, uint32_t bg);

void kernel_main(void){
    if (fb_init(800, 600) == 0) {
        draw_str(16, 16, "Hello, world!", 0xFFFFFFFFu /* white */, 0x00000000u /* black */);
    }
    for(;;) __asm__ __volatile__("wfi");
}
```

That’s it: build, copy `kernel8.img` to the SD card’s boot partition with the firmware files, and power on. The key moving parts are (a) mailbox property buffer alignment and tags, and (b) remembering the framebuffer pointer is a **GPU bus address** that you typically mask to an ARM physical address before dereferencing. [Raspberry Pi 4 Operating System](https://www.rpi4os.com/part5-framebuffer/)

------

## Notes & gotchas

- **Channel and alignment:** Mailbox property channel is **8**; the buffer must be 16-byte aligned; only the upper 28 bits of the pointer go over the mailbox (low 4 bits carry the channel). [Raspberry Pi 4 Operating System](https://www.rpi4os.com/part5-framebuffer/)
- **Where does the kernel start?** Bare-metal tutorials commonly link **AArch64** at `0x80000` and **AArch32** at `0x8000`. The boot firmware loads your kernel at that address and jumps to it. [OSDev Wiki](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones?utm_source=chatgpt.com)[Raspberry Pi Stack Exchange](https://raspberrypi.stackexchange.com/questions/26442/does-the-kernel-img-file-loaded-at-physical-0x00000000-or-0x00008000?utm_source=chatgpt.com)
- **Depth & pitch:** Ask for **32-bpp**; then read back **pitch** (bytes per scanline) from the property reply. [Valvers](https://www.valvers.com/open-software/raspberry-pi/bare-metal-programming-in-c-part-5/?utm_source=chatgpt.com)
- **AArch32 variant:** Use `kernel.img`, a 32-bit startup stub, and link at `0x8000`. Mailbox/tags are the same; only the assembly calling convention and SP init change. [Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=187136&utm_source=chatgpt.com)

If you want, I can package this into a tiny ready-to-build repo (Makefile + font table + linker + stubs) next.



##### You said:

can we plan for AARch32 because then we can use this on our eventual emulator?



###### ChatGPT said:

Thought for 28s

Absolutely—let’s target **AArch32 (ARMv7)** so the same “Hello, world” payload can also run inside your emulator later.

Here’s a compact, drop-in mini project for **Raspberry Pi 3 (AArch32)** that:

- boots at **0x8000**,
- uses the **mailbox property interface (channel 8)** to allocate a **32-bpp framebuffer**,
- draws “Hello, world!” with an 8×8 bitmap font,
- then loops with `wfi`.

I’ve kept paths/filenames conventional for Pi 2/3:

- **kernel name:** `kernel7.img` (ARMv7 32-bit) [Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=187136&utm_source=chatgpt.com)
- **peripheral base:** `0x3F000000` (Pi 2/3) and mailbox at base+`0xB880` [OSDev Wiki](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones?utm_source=chatgpt.com)
- **framebuffer tags:** set physical size `0x00048003`, virtual size `0x00048004`, depth `0x00048005`, alloc buffer `0x00040001`, get pitch `0x00040008` (property channel **8**) [Raspberry Pi OS+1](https://jsandler18.github.io/extra/prop-channel.html?utm_source=chatgpt.com)

------

# Files

### `link.ld`

```
ldCopyEditENTRY(_start)
SECTIONS {
  . = 0x8000;          /* Pi 2/3, AArch32 entry */
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }
  _stack_top = . + 0x4000; /* tiny stack after .bss (adjust as you like) */
}
```

### `start.S` (AArch32)

```
asmCopyEdit    .global _start
    .section .text

_start:
    ldr   r0, =_stack_top
    mov   sp, r0

    bl    kernel_main

1:  wfi
    b     1b
```

### `mailbox.h`

```
cCopyEdit#pragma once
#include <stdint.h>

#define PERI_BASE     0x3F000000u      /* Pi 2/3 */   /* Pi 4 is 0xFE000000 */
#define MBOX_BASE     (PERI_BASE + 0xB880)
#define MBOX_READ     (MBOX_BASE + 0x00)
#define MBOX_STATUS   (MBOX_BASE + 0x18)
#define MBOX_WRITE    (MBOX_BASE + 0x20)
#define MBOX_FULL     0x80000000u
#define MBOX_EMPTY    0x40000000u
#define MBOX_CH_PROP  8                /* property interface channel */

static inline void mmio_write(uint32_t r, uint32_t v){ *(volatile uint32_t*)r = v; }
static inline uint32_t mmio_read(uint32_t r){ return *(volatile uint32_t*)r; }

/* 16-byte aligned, only upper 28 bits of address go through mailbox */
extern volatile uint32_t __attribute__((aligned(16))) mbox[];
int mbox_call(unsigned ch);

/* framebuffer info obtained from property calls */
extern uint8_t  *fb;
extern uint32_t  fb_pitch, fb_w, fb_h;

int fb_init(unsigned w, unsigned h);   /* returns 0 on success */
```

### `mailbox.c`

```
cCopyEdit#include "mailbox.h"

volatile uint32_t __attribute__((aligned(16))) mbox[36];
uint8_t  *fb;
uint32_t  fb_pitch, fb_w, fb_h;

int mbox_call(unsigned ch) {
    uint32_t addr = ((uint32_t)(uintptr_t)mbox & ~0xF) | (ch & 0xF);
    while (mmio_read(MBOX_STATUS) & MBOX_FULL) {}
    mmio_write(MBOX_WRITE, addr);
    for (;;) {
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY) {}
        if (mmio_read(MBOX_READ) == addr)
            return mbox[1] == 0x80000000; /* response OK */
    }
}

/* Ask VC for a 32bpp framebuffer and read back address & pitch */
int fb_init(unsigned w, unsigned h) {
    fb = 0; fb_pitch = 0; fb_w = w; fb_h = h;

    mbox[0]  = 35*4;         /* total size */
    mbox[1]  = 0;            /* request */

    mbox[2]  = 0x00048003;   /* set physical size */
    mbox[3]  = 8; mbox[4]=8;
    mbox[5]  = w; mbox[6]=h;

    mbox[7]  = 0x00048004;   /* set virtual size */
    mbox[8]  = 8; mbox[9]=8;
    mbox[10] = w; mbox[11]=h;

    mbox[12] = 0x00048005;   /* set depth */
    mbox[13] = 4; mbox[14]=4;
    mbox[15] = 32;

    mbox[16] = 0x00040001;   /* alloc fb */
    mbox[17] = 8; mbox[18]=8;
    mbox[19] = 4096;         /* alignment */
    mbox[20] = 0;

    mbox[21] = 0x00040008;   /* get pitch */
    mbox[22] = 4; mbox[23]=4;
    mbox[24] = 0;

    mbox[25] = 0;            /* end */

    if (!mbox_call(MBOX_CH_PROP)) return -1;

    /* Framebuffer address is returned as a bus address: mask to ARM phys */
    uint32_t fb_bus = mbox[20] ? mbox[20] : mbox[19];
    fb       = (uint8_t*)(uintptr_t)(fb_bus & 0x3FFFFFFFu);
    fb_pitch = mbox[24];
    return (fb && fb_pitch) ? 0 : -1;
}
```

(Tag IDs & property channel details: property mailbox channel **8**, with the above tag IDs. [Raspberry Pi OS+1](https://jsandler18.github.io/extra/prop-channel.html?utm_source=chatgpt.com))

### `font8x8_basic.h`

Use the tiny public-domain 8×8 font (drop-in header):
 `#include "font8x8_basic.h"` from **dhepper/font8x8**. [GitHub+1](https://github.com/dhepper/font8x8?utm_source=chatgpt.com)

### `draw.c`

```
cCopyEdit#include <stdint.h>
#include "mailbox.h"
#include "font8x8_basic.h"

static inline void putpx(int x,int y,uint32_t argb){
    *(volatile uint32_t*)(fb + y*fb_pitch + x*4) = argb;
}

static void draw_char(int x,int y,char c,uint32_t fg,uint32_t bg){
    const unsigned char *g = font8x8_basic[(unsigned char)c];
    for(int r=0;r<8;r++){
        unsigned bits = g[r];
        for(int c2=0;c2<8;c2++){
            putpx(x+c2, y+r, (bits & (1u<<c2)) ? fg : bg);
        }
    }
}

void draw_str(int x,int y,const char* s,uint32_t fg,uint32_t bg){
    for(int i=0;s[i];i++) draw_char(x+i*8, y, s[i], fg, bg);
}
```

### `main.c`

```
cCopyEdit#include <stdint.h>
#include "mailbox.h"

/* from draw.c */
void draw_str(int x,int y,const char* s,uint32_t fg,uint32_t bg);

void kernel_main(void){
    if (fb_init(800, 600) == 0) {
        draw_str(16,16, "Hello, world!", 0xFFFFFFFF, 0x00000000);
    }
    for(;;) __asm__ __volatile__("wfi");
}
```

### `Makefile`

```
makeCopyEditCROSS ?= arm-none-eabi
CC    := $(CROSS)-gcc
AS    := $(CROSS)-as
LD    := $(CROSS)-ld
OBJC  := $(CROSS)-objcopy
CFLAGS:= -O2 -ffreestanding -nostdlib -nostartfiles -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
LDFLAGS:= -T link.ld

OBJS = start.o mailbox.o draw.o main.o

all: kernel7.img

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,--build-id=none -o $@ $(OBJS)

kernel7.img: kernel.elf
	$(OBJC) -O binary $< $@

clean:
	rm -f $(OBJS) kernel.elf kernel7.img
```

### SD card (FAT) layout

- `kernel7.img` (the binary you built)

- `config.txt`:

  ```
  iniCopyEditarm_64bit=0
  kernel=kernel7.img
  enable_uart=1
  ```

  (`kernel7.img` is the standard 32-bit name on Pi 2/3; `arm_64bit=0` enforces 32-bit mode if your firmware defaults to 64-bit.) [Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=187136&utm_source=chatgpt.com)

------

## Why this maps cleanly to your emulator

- It touches only **mailbox property** registers (MMIO at `0x3F00_B880`), **no GPU specifics**. Your emulator can stub the property calls (e.g., on `MBOX_WRITE/READ`) and hand back a fake framebuffer pointer/pitch—exactly the same way you already model UART/CRT.
- The CPU side is just AArch32 instructions (your VM already decodes `WFI` as a no-op message).
- If you don’t emulate the mailbox yet, you can quickly replace `fb_init()` with a direct pointer to your emulator’s CRT VRAM and keep everything else identical.

If you want, I can also give you a minimal **mailbox stub implementation for your VM** so this same binary renders into your emulator’s CRT buffer without touching real hardware.

**References:** Raspberry Pi 2/3 peripheral base and mailbox location; property mailbox tags for framebuffer; kernel image naming conventions. [OSDev Wiki](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones?utm_source=chatgpt.com)[Raspberry Pi OS+1](https://jsandler18.github.io/extra/mailbox.html?utm_source=chatgpt.com)[Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=187136&utm_source=chatgpt.com)