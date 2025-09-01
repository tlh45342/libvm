can you give me the code to query the display size.  If I am rendering glyps as pixels I am assuming there is a way to write pixels to the display.  Can you give me the code to write pixels to the screen?



###### ChatGPT said:

Thought for 6s

Yep! Easiest path (and perfect for virtio‑gpu or simplefb) is to use the Linux framebuffer device `/dev/fb0`. This lets you:

1. query width/height/pixel format, and
2. mmap the buffer and write pixels directly.

Here’s a compact C example that:

- prints the resolution and format
- paints a few colored pixels and a filled rectangle

> Works on ARMv7/AArch64 with fbdev enabled (virtio‑gpu usually exposes fbdev). If `/dev/fb0` doesn’t exist on your image, say the word and I’ll give you a DRM/KMS (libdrm) version.

```
cCopyEdit// fb_demo.c
// Build: gcc -O2 -Wall -o fb_demo fb_demo.c
// Run as root (needs write to /dev/fb0)

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <errno.h>

static inline uint32_t pack_pixel(uint32_t r, uint32_t g, uint32_t b,
                                  struct fb_var_screeninfo *vinfo) {
    // Map 8-bit RGB into the framebuffer’s declared channel layout.
    // Common virtio-gpu fbdev is XRGB8888 (32bpp), but this handles offsets/lengths.
    uint32_t px = 0;
    if (vinfo->bits_per_pixel == 32) {
        px |= ((r >> (8 - vinfo->red.length))   & ((1u << vinfo->red.length)   - 1)) << vinfo->red.offset;
        px |= ((g >> (8 - vinfo->green.length)) & ((1u << vinfo->green.length) - 1)) << vinfo->green.offset;
        px |= ((b >> (8 - vinfo->blue.length))  & ((1u << vinfo->blue.length)  - 1)) << vinfo->blue.offset;
    } else if (vinfo->bits_per_pixel == 24) {
        // Rare on modern VMs, but possible; write as packed 3 bytes (B,G,R) order depends on offsets.
        // We’ll still return a 32-bit value; caller writes 3 bytes.
        px |= ((r >> (8 - vinfo->red.length))   & ((1u << vinfo->red.length)   - 1)) << vinfo->red.offset;
        px |= ((g >> (8 - vinfo->green.length)) & ((1u << vinfo->green.length) - 1)) << vinfo->green.offset;
        px |= ((b >> (8 - vinfo->blue.length))  & ((1u << vinfo->blue.length)  - 1)) << vinfo->blue.offset;
    } else if (vinfo->bits_per_pixel == 16) {
        // Assume RGB565-like; map down.
        uint32_t R = r >> (8 - vinfo->red.length);
        uint32_t G = g >> (8 - vinfo->green.length);
        uint32_t B = b >> (8 - vinfo->blue.length);
        px = (R << vinfo->red.offset) | (G << vinfo->green.offset) | (B << vinfo->blue.offset);
    } else {
        // Unsupported bpp for this demo; fall back to black.
        px = 0;
    }
    return px;
}

int main(void) {
    const char *fbpath = "/dev/fb0";
    int fb = open(fbpath, O_RDWR);
    if (fb < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", fbpath, strerror(errno));
        return 1;
    }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("FBIOGET_FSCREENINFO");
        close(fb);
        return 1;
    }
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOGET_VSCREENINFO");
        close(fb);
        return 1;
    }

    printf("Resolution: %ux%u (virtual %ux%u), bpp=%u, line_length=%u bytes\n",
           vinfo.xres, vinfo.yres, vinfo.xres_virtual, vinfo.yres_virtual,
           vinfo.bits_per_pixel, finfo.line_length);
    printf("RGB offsets/lengths: R(%u,%u) G(%u,%u) B(%u,%u)\n",
           vinfo.red.offset, vinfo.red.length,
           vinfo.green.offset, vinfo.green.length,
           vinfo.blue.offset, vinfo.blue.length);

    // mmap the framebuffer
    size_t screensize = finfo.line_length * vinfo.yres_virtual;
    uint8_t *fbmem = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fbmem == MAP_FAILED) {
        perror("mmap");
        close(fb);
        return 1;
    }

    // Helper to set one pixel at (x,y)
    auto void put_pixel(unsigned x, unsigned y, uint32_t r, uint32_t g, uint32_t b) {
        if (x >= vinfo.xres || y >= vinfo.yres) return;
        uint32_t pixel = pack_pixel(r,g,b, &vinfo);
        uint8_t *row = fbmem + y * finfo.line_length;
        switch (vinfo.bits_per_pixel) {
            case 32:
                ((uint32_t*)row)[x] = pixel;
                break;
            case 24: {
                // Write 3 bytes at the correct byte order (derive from offsets).
                // Most XRGB8888-like 24bpp will be B,G,R in memory if blue.offset==0.
                uint8_t *p = row + x * 3;
                uint8_t R = (pixel >> vinfo.red.offset)   & ((1u << vinfo.red.length)   - 1);
                uint8_t G = (pixel >> vinfo.green.offset) & ((1u << vinfo.green.length) - 1);
                uint8_t B = (pixel >> vinfo.blue.offset)  & ((1u << vinfo.blue.length)  - 1);
                // Normalize back to 8 bits for writing
                // (If lengths <8 we scale up; for demo, simple placement is fine)
                p[0] = B; p[1] = G; p[2] = R;
                break;
            }
            case 16: {
                uint16_t *p = (uint16_t*)row + x;
                *p = (uint16_t)pixel;
                break;
            }
            default:
                break;
        }
    }

    // Clear screen to dark gray
    for (unsigned y = 0; y < vinfo.yres; y++) {
        for (unsigned x = 0; x < vinfo.xres; x++) {
            put_pixel(x, y, 30, 30, 30);
        }
    }

    // Draw a few colored pixels and a rectangle
    put_pixel(10, 10, 255, 0, 0);
    put_pixel(11, 10, 0, 255, 0);
    put_pixel(12, 10, 0, 0, 255);

    // Filled rectangle at (50,50) size 200x100, color teal
    for (unsigned y = 50; y < 150 && y < vinfo.yres; y++) {
        for (unsigned x = 50; x < 250 && x < vinfo.xres; x++) {
            put_pixel(x, y, 0, 180, 180);
        }
    }

    // Keep it visible a bit (optional)
    usleep(500 * 1000);

    munmap(fbmem, screensize);
    close(fb);
    return 0;
}
```

### How this answers your questions

- **“Query the display size”** → the `FBIOGET_VSCREENINFO` ioctl gives you `xres`, `yres`, `bits_per_pixel`, and the RGB channel layout; `FBIOGET_FSCREENINFO` gives `line_length` (stride).
- **“Write pixels to the screen”** → you `mmap()` the framebuffer and compute:
   `addr = fb_base + y * line_length + x * (bpp/8)` (with proper packing), then store your pixel.

### If you end up without `/dev/fb0`

Modern distros sometimes use DRM/KMS without fbdev. On virtio‑gpu, fbdev **usually** exists, but if not, I can give you a **libdrm** example that:

- opens `/dev/dri/card0`,
- selects a connector/mode (width/height),
- creates a **“dumb buffer”**, maps it, and
- fills pixels similarly (then does a modeset).

Want that DRM/KMS version too?