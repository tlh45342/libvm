let's assume barebones and we are writing to hardware directly because we don't have any of the convenient common linux code.  We are in the process of exploring this phenomena which probably also teaches us to appreciate modular concepts.  So, if using assembly; how would one query the screen size;  let me know if there are any assumptions  we need to discuss because there may be certain things we do need to handle in advance; DTB being one?  Just let me know and we can create a safe place to start from.



###### ChatGPT said:

Thought for 13s

Great—let’s make a **bare‑metal, assembly‑first** plan that doesn’t assume any Linux helpers. There are two sane “starting points” for video on an ARMv7/QEMU‑virt‑like machine:

1. **SimpleFB via DTB** (easiest): firmware/your VM advertises a pre‑initialized linear framebuffer in the **Device Tree**. You just read **width/height/stride/addr** from DTB and start writing pixels.
2. **virtio‑gpu (mmio)** (harder): you bring up the virtio transport, create a 2D resource, and query **GET_DISPLAY_INFO** to learn the current scanout size. More moving parts, but it’s the “real” device path.

If your goal is to “learn the plumbing” and quickly draw pixels, **start with SimpleFB via DTB**. We can layer virtio‑gpu later.

------

# Assumptions (say yes/no and we’ll tweak)

- **CPU:** ARMv7 (AArch32), little‑endian.
- **RAM base:** 0x4000_0000 (typical “virt” style).
- **DTB pointer:** you (the boot ROM/VM) place the **FDT (flattened device tree)** pointer in a known register; let’s pick **r2** (Linux uses r2—handy convention).
- **Framebuffer model:** DTB contains a node with `compatible = "simple-framebuffer"` and properties:
  - `reg` (base + size)
  - `width`, `height`, `stride` (bytes per line)
  - `format` (e.g., `"r8g8b8a8"` or `"a8r8g8b8"`)
- **MMIO addresses:** discovered via DTB (we won’t hardcode).

If those work, here’s a minimal path.

------

# Option A: SimpleFB via DTB (query size + write pixels)

## 1) Tiny FDT parser in ARMv7 assembly (read `width/height/stride/reg`)

FDT layout you need:

- Header (big‑endian fields):
   `magic(0xD00DFEED)`, `off_dt_struct`, `off_dt_strings`, `size_dt_struct`, …
- **Structure block** has tokens:
   `FDT_BEGIN_NODE (1)`, `FDT_END_NODE (2)`, `FDT_PROP (3)`, `FDT_NOP (4)`, `FDT_END (9)`
- `FDT_PROP` payload: `u32 len`, `u32 nameoff`, then `len` bytes of data. `nameoff` indexes into the **strings block**.

Strategy:

- Walk nodes; when you see a node that has `compatible` containing `"simple-framebuffer"`, record its properties.

Below is compact **ARMv7 assembly pseudo‑code** (GNU syntax). It’s not a full FDT lib, but shows the essence.

```
asmCopyEdit        .equ FDT_MAGIC,     0xD00DFEED
        .equ FDT_BEGIN_NODE,1
        .equ FDT_END_NODE,  2
        .equ FDT_PROP,      3
        .equ FDT_NOP,       4
        .equ FDT_END,       9

        .text
        .global fb_query_from_dtb
        // in: r2 = pointer to FDT blob
        // out: r0=fb_base, r1=stride, r3=height, r4=width
fb_query_from_dtb:
        push    {r4-r11,lr}

        // Load header fields (big-endian → convert)
        ldr     r5, [r2, #0]              // magic (BE)
        bl      be32_to_cpu
        cmp     r0, #FDT_MAGIC
        bne     .bad

        ldr     r6, [r2, #4]              // totalsize (unused)
        ldr     r7, [r2, #8]              // off_dt_struct
        ldr     r8, [r2, #0x10]           // off_dt_strings
        bl      be32_to_cpu_r7r8          // convert r7,r8 to LE

        add     r7, r2, r7                // struct_ptr = fdt + off_dt_struct
        add     r8, r2, r8                // strings    = fdt + off_dt_strings

        mov     r9, #0                    // found_simplefb = 0
        mov     r0, #0                    // fb_base = 0 (default)
        mov     r1, #0                    // stride
        mov     r3, #0                    // height
        mov     r4, #0                    // width

// iterate tokens
.next_token:
        ldr     r10, [r7], #4
        bl      be32_to_cpu_inplace       // r10 = token
        cmp     r10, #FDT_BEGIN_NODE
        beq     .begin_node
        cmp     r10, #FDT_PROP
        beq     .prop
        cmp     r10, #FDT_END_NODE
        beq     .end_node
        cmp     r10, #FDT_END
        beq     .done
        // FDT_NOP or unknown → skip
        b       .next_token

.begin_node:
        // name is a NUL-terminated string at r7; skip it (align to 4)
        // (We don't need the node name—only properties, especially 'compatible'.)
        mov     r11, r7
.skip_name:
        ldrb    r12, [r11], #1
        cmp     r12, #0
        bne     .skip_name
        // align r11 to 4
        add     r11, r11, #3
        bic     r11, r11, #3
        mov     r7, r11
        b       .next_token

.prop:
        // read len, nameoff
        ldr     r12, [r7], #4         // len (BE)
        bl      be32_to_cpu_inplace
        ldr     lr,  [r7], #4         // nameoff (BE)
        // convert nameoff
        mov     r0, lr
        bl      be32_to_cpu
        mov     lr, r0
        add     r0, r8, lr            // name = strings + nameoff
        // compare property name
        // We care about: "compatible", "reg", "width", "height", "stride"
        bl      strcmp_compatible
        cmp     r0, #0
        beq     .prop_is_compatible

        bl      strcmp_reg
        cmp     r0, #0
        beq     .prop_is_reg

        bl      strcmp_width
        cmp     r0, #0
        beq     .prop_is_width

        bl      strcmp_height
        cmp     r0, #0
        beq     .prop_is_height

        bl      strcmp_stride
        cmp     r0, #0
        beq     .prop_is_stride

        // Skip data (len bytes), align to 4
        add     r7, r7, r12
        add     r7, r7, #3
        bic     r7, r7, #3
        b       .next_token

.prop_is_compatible:
        // data at r7, length r12. Check if it contains "simple-framebuffer"
        mov     r0, r7
        mov     r1, r12
        bl      contains_simplefb
        mov     r9, r0                  // r0=1 if found
        // advance
        add     r7, r7, r12
        add     r7, r7, #3
        bic     r7, r7, #3
        b       .next_token

.prop_is_reg:
        // Expect reg: <base size> in 32/64-bit cells per #address-cells/#size-cells.
        // For QEMU-virt style we use 2 cells each → 64-bit. We'll read the first 64-bit base, ignore size.
        ldr     r0, [r7], #4            // base_hi (BE)
        bl      be32_to_cpu_inplace
        ldr     r2, [r7], #4            // base_lo (BE)
        bl      be32_to_cpu_inplace
        // ignore size cells for brevity: skip next 8 bytes
        add     r7, r7, #8
        // Combine to 32-bit if high is zero
        cmp     r0, #0
        moveq   r0, r2
        // save fb_base only if we've identified simplefb in this node (r9==1)
        cmp     r9, #1
        streq   r0, [sp, #-4]!          // stash on stack temporarily
        ldreq   r0, [sp], #4
        moveq   r0, r0                   // (no-op; shows intent)
        // advance/align
        sub     r7, r7, #8               // we advanced above; but we've read 16 bytes total for reg
        add     r7, r7, r12              // r12 was total len; align next
        add     r7, r7, #3
        bic     r7, r7, #3
        b       .next_token

.prop_is_width:
        ldr     r0, [r7]                 // width (BE u32)
        bl      be32_to_cpu_inplace
        cmp     r9, #1
        moveq   r4, r0
        add     r7, r7, r12
        add     r7, r7, #3
        bic     r7, r7, #3
        b       .next_token

.prop_is_height:
        ldr     r0, [r7]
        bl      be32_to_cpu_inplace
        cmp     r9, #1
        moveq   r3, r0
        add     r7, r7, r12
        add     r7, r7, #3
        bic     r7, r7, #3
        b       .next_token

.prop_is_stride:
        ldr     r0, [r7]
        bl      be32_to_cpu_inplace
        cmp     r9, #1
        moveq   r1, r0
        add     r7, r7, r12
        add     r7, r7, #3
        bic     r7, r7, #3
        b       .next_token

.end_node:
        // Leaving node: clear found_simplefb if set
        mov     r9, #0
        b       .next_token

.done:
        // r0=fb_base, r1=stride, r3=height, r4=width (as we filled above)
        pop     {r4-r11,pc}

.bad:
        mov     r0, #0
        mov     r1, #0
        mov     r3, #0
        mov     r4, #0
        pop     {r4-r11,pc}

/* --- helper stubs you’ll implement: --- */
// r0=BE32 → return LE32 in r0
be32_to_cpu:
        rev     r0, r5      // example; adapt to your calling
        bx      lr

be32_to_cpu_inplace:
        rev     r10, r10
        bx      lr

be32_to_cpu_r7r8:
        rev     r7, r7
        rev     r8, r8
        bx      lr

strcmp_compatible:   // r0 = name ptr
        // compare to "compatible"
        // return 0 if equal
        // (left as an exercise; short inline strcmp)
        bx      lr

/* contains_simplefb:
   in: r0 = data ptr, r1 = len
   out: r0 = 1 if the compatible string list contains "simple-framebuffer" */
contains_simplefb:
        // scan NUL‑separated strings for substring match
        bx      lr
```

> That’s intentionally minimal. If you’d rather, I can give you a tiny C FDT reader you can call from assembly.

## 2) Writing pixels once you have (base, width, height, stride)

Assume **32‑bpp XRGB** and a solid color fill (ARM assembly):

```
asmCopyEdit        // in: r0=fb_base, r1=stride (bytes), r4=width, r3=height
        // color in r2 = 0x00RRGGBB (or whatever your format is)
fill_screen:
        push    {r4-r8,lr}
        mov     r5, #0              // y
1:      cmp     r5, r3
        bhs     3f
        // row_ptr = base + y*stride
        mul     r6, r5, r1
        add     r7, r0, r6
        mov     r8, #0              // x
2:      cmp     r8, r4
        bhs     4f
        // store 32-bit pixel at row_ptr + x*4
        add     r6, r7, r8, lsl #2
        str     r2, [r6]
        add     r8, r8, #1
        b       2b
4:      add     r5, r5, #1
        b       1b
3:      pop     {r4-r8,pc}
```

From there, you can blit glyphs by copying pre‑rasterized bitmaps into the mapped framebuffer rows using the same addressing.

------

# Option B: virtio‑gpu (query size + draw)

If you want to learn virtio, the flow (still assembly‑friendly) is:

1. **Discover base** for `virtio,mmio` GPU node in DTB (`reg` + IRQ).
2. **Reset & status:** write to **Device Status** register: `ACKNOWLEDGE → DRIVER → (FEATURES_OK) → DRIVER_OK`.
3. **Feature negotiation:** read **Device Features**, mask what you support, write **Driver Features**, set **FEATURES_OK**.
4. **Setup virtqueues:** for the **control queue** (and cursor if desired). Allocate descriptor ring, avail, used; write addresses to MMIO; enable.
5. **GET_DISPLAY_INFO:** push a command on the control queue (`VIRTIO_GPU_CMD_GET_DISPLAY_INFO`), kick; poll used ring; parse `r.width/r.height` for scanout 0.
6. **Create 2D resource** at that size, **ATTACH_BACKING** to guest memory, **SET_SCANOUT**, then **TRANSFER_TO_HOST_2D** / **RESOURCE_FLUSH** to present pixels.

That’s 100–200 lines of careful MMIO + ring code once you have a virtio‑mmio skeleton. It’s doable, but I’d still recommend **SimpleFB first** so you can see pixels N hours sooner.

------

# “Safe place to start” checklist

-  Decide how your **boot ROM** passes the **DTB pointer** (let’s standardize on **r2**).
-  Add a **simple‑framebuffer** node to your VM’s DTB with valid `reg/width/height/stride/format`.
-  Write the tiny **FDT parser** (or call a C helper) to extract those properties.
-  Implement the **fill_screen** routine and a **put_glyph** that blits an 8×16 font.
-  Later: swap SimpleFB for **virtio‑gpu**; your drawing code won’t change much—only how you get the surface.

If you want, give me your **current DTB (or the DTS you intend)** and I’ll adjust the parsing and the constants exactly to it, plus provide a tiny known‑good 8×16 font blob and a `putc` that draws ASCII to the screen.