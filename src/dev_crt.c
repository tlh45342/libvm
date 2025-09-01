#include "dev_crt.h"

#include <stdio.h>
#include <string.h>
#include "mem.h"      // mem_read8
#include "wincrt.h"   // wincrt_init_text, wincrt_present_text, wincrt_pump_messages

#ifdef _WIN32
  #include <windows.h>
#else
  #include <time.h>
  #include <sys/time.h>
#endif

// --------------------- local state ---------------------

static struct {
    bool     inited;
    bool     enabled;
    bool     dirty;
    int      cols, rows;
    uint32_t base;          // guest VRAM base for text (each cell = 2 bytes)
    uint64_t last_ns;
} g_crt;

static uint64_t host_now_ns(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER ctr;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ctr);
    return (uint64_t)((__int128)ctr.QuadPart * 1000000000ull / (uint64_t)freq.QuadPart);
#else
  #ifdef CLOCK_MONOTONIC
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  #else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000ull + (uint64_t)tv.tv_usec * 1000ull;
  #endif
#endif
}

// --------------------- public API ----------------------

bool dev_crt_init(int cols, int rows, uint32_t mmio_text_base) {
    if (cols <= 0 || rows <= 0) return false;
    memset(&g_crt, 0, sizeof(g_crt));
    g_crt.cols = cols;
    g_crt.rows = rows;
    g_crt.base = mmio_text_base;
    g_crt.inited = true;
    g_crt.last_ns = host_now_ns();
    return true;
}

void dev_crt_set_enabled(bool enabled) {
    if (!g_crt.inited) return;
    if (enabled && !g_crt.enabled) {
        // Create the OS window once
        wincrt_init_text(g_crt.cols, g_crt.rows);
        g_crt.dirty = true;
    }
    g_crt.enabled = enabled;
}

bool dev_crt_is_enabled(void) {
    return g_crt.inited && g_crt.enabled;
}

void dev_crt_mark_dirty(uint32_t addr, size_t len) {
    if (!g_crt.inited) return;
    const uint32_t start = g_crt.base;
    const uint32_t end   = g_crt.base + (uint32_t)(g_crt.cols * g_crt.rows * 2);
    if (addr < end && (addr + (uint32_t)len) > start) {
        g_crt.dirty = true;
    }
}

void dev_crt_present_now(void) {
    if (!g_crt.inited || !g_crt.enabled) return;

    const int VRAM_BYTES = g_crt.cols * g_crt.rows * 2;
    static uint8_t vbuf[/*max*/ 200*100*2]; // enough for up to 200x100
    if (VRAM_BYTES > (int)sizeof(vbuf)) return; // safety

    for (int i = 0; i < VRAM_BYTES; i++) {
        vbuf[i] = mem_read8(g_crt.base + (uint32_t)i);
    }
    wincrt_present_text(vbuf, 0, 0, g_crt.cols, g_crt.rows);
    g_crt.dirty = false;
}

void dev_crt_pump_60hz(void) {
    if (!g_crt.inited || !g_crt.enabled) return;
    wincrt_pump_messages();
    const uint64_t now = host_now_ns();
    const uint64_t interval = 16ull * 1000ull * 1000ull; // ~60Hz
    if (g_crt.dirty || (now - g_crt.last_ns) >= interval) {
        dev_crt_present_now();
        g_crt.last_ns = now;
    }
}

void dev_crt_dump_text(void) {
    if (!g_crt.inited) return;
    const int cols = g_crt.cols, rows = g_crt.rows;
    printf("\n==== CRT OUTPUT ====\n");
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            uint32_t off = g_crt.base + (uint32_t)((r * cols + c) * 2);
            uint8_t ch = mem_read8(off);
            putchar((ch >= 32 && ch < 127) ? (char)ch : '.');
        }
        putchar('\n');
    }
    printf("====================\n");
}

void dev_crt_shutdown(void) {
    // Nothing to free for now (wincrt_* manages its own window lifetime)
    // Keep function for symmetry / future resources.
}