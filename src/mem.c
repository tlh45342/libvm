// src/mem.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "mem.h"
#include "dev_disk.h"   // dev_disk0_present(), dev_disk0_read_reg(), dev_disk0_write_reg()
#include "dev_uart.h"   // dev_uart_present(), dev_uart_read_reg(), dev_uart_write_reg()

// ==========================
// Internal RAM state
// ==========================
static uint8_t *g_ram_base = NULL;
static size_t   g_ram_size = 0;
static bool     g_ram_bound = false;

// ==========================
// MMIO windows (word-based)
// ==========================
#define DISK0_BASE 0x0B000000u
#define DISK0_SIZE 0x00001000u

#define UART0_BASE 0x09000000u
#define UART0_SIZE 0x00001000u

static inline bool in_disk0(uint32_t a) {
    return (uint32_t)(a - DISK0_BASE) < DISK0_SIZE;
}
static inline bool in_uart0(uint32_t a) {
    return (uint32_t)(a - UART0_BASE) < UART0_SIZE;
}

// Disk0 32-bit MMIO accessors (thin wrappers)
static inline uint32_t disk0_rd32(uint32_t addr) {
    return dev_disk0_present() ? dev_disk0_read_reg(addr & ~3u) : 0u;
}
static inline void disk0_wr32(uint32_t addr, uint32_t val) {
    if (dev_disk0_present()) dev_disk0_write_reg(addr & ~3u, val);
}

// ==========================
// RAM helpers (bounds-safe)
// ==========================
static inline bool ram_ok(uint32_t addr, size_t len) {
    if (!g_ram_bound) return false;
    if ((size_t)addr > g_ram_size) return false;
    if (len > g_ram_size - (size_t)addr) return false;
    return true;
}

static inline uint8_t ram_read8(uint32_t addr) {
    return ram_ok(addr, 1) ? g_ram_base[addr] : 0;
}

static inline void ram_write8(uint32_t addr, uint8_t v) {
    if (ram_ok(addr, 1)) g_ram_base[addr] = v;
}

static inline uint32_t ram_read32(uint32_t addr) {
    if (!ram_ok(addr, 4)) return 0;
    // Little-endian, allow unaligned safely
    uint32_t b0 = g_ram_base[addr + 0];
    uint32_t b1 = g_ram_base[addr + 1];
    uint32_t b2 = g_ram_base[addr + 2];
    uint32_t b3 = g_ram_base[addr + 3];
    return (b0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

static inline void ram_write32(uint32_t addr, uint32_t v) {
    if (!ram_ok(addr, 4)) return;
    g_ram_base[addr + 0] = (uint8_t)(v & 0xFF);
    g_ram_base[addr + 1] = (uint8_t)((v >> 8) & 0xFF);
    g_ram_base[addr + 2] = (uint8_t)((v >> 16) & 0xFF);
    g_ram_base[addr + 3] = (uint8_t)((v >> 24) & 0xFF);
}

// ==========================
// Public API (mem.h)
// ==========================
void mem_init(void) {
    g_ram_base  = NULL;
    g_ram_size  = 0;
    g_ram_bound = false;
}

void mem_bind(uint8_t *base, size_t size) {
    g_ram_base  = base;
    g_ram_size  = size;
    g_ram_bound = (base != NULL && size > 0);
}

void mem_unbind(void) {
    g_ram_base  = NULL;
    g_ram_size  = 0;
    g_ram_bound = false;
}

bool mem_is_bound(void) {
    return g_ram_bound;
}

size_t mem_size(void) {
    return g_ram_size;
}

// ---------- Reads ----------
uint8_t mem_read8(uint32_t addr) {
    // UART (byte via read-modify of 32-bit reg)
    if (in_uart0(addr) && dev_uart_present()) {
        uint32_t base  = addr & ~3u;
        uint32_t shift = (addr & 3u) * 8u;
        uint32_t w     = dev_uart_read_reg(base);
        return (uint8_t)((w >> shift) & 0xFFu);
    }
    // DISK (byte via read-modify of 32-bit reg)
    if (in_disk0(addr) && dev_disk0_present()) {
        uint32_t base  = addr & ~3u;
        uint32_t shift = (addr & 3u) * 8u;
        uint32_t w     = disk0_rd32(base);
        return (uint8_t)((w >> shift) & 0xFFu);
    }
    // RAM
    return ram_read8(addr);
}

uint32_t mem_read32(uint32_t addr) {
    if (in_uart0(addr) && dev_uart_present()) {
        return dev_uart_read_reg(addr);
    }
    if (in_disk0(addr) && dev_disk0_present()) {
        return disk0_rd32(addr);
    }
    return ram_read32(addr);
}

// ---------- Writes ----------
void mem_write8(uint32_t addr, uint8_t v) {
    // UART (byte lane write as RMW of 32-bit reg)
    if (in_uart0(addr) && dev_uart_present()) {
        uint32_t base  = addr & ~3u;
        uint32_t shift = (addr & 3u) * 8u;
        uint32_t cur   = dev_uart_read_reg(base);
        uint32_t mask  = 0xFFu << shift;
        uint32_t merged = (cur & ~mask) | ((uint32_t)v << shift);
        dev_uart_write_reg(base, merged);
        return;
    }
    // DISK (byte lane write as RMW of 32-bit reg)
    if (in_disk0(addr) && dev_disk0_present()) {
        uint32_t base  = addr & ~3u;
        uint32_t shift = (addr & 3u) * 8u;
        uint32_t cur   = disk0_rd32(base);
        uint32_t mask  = 0xFFu << shift;
        uint32_t merged = (cur & ~mask) | ((uint32_t)v << shift);
        disk0_wr32(base, merged);
        return;
    }
    // RAM
    ram_write8(addr, v);
}

void mem_write32(uint32_t addr, uint32_t v) {
    if (in_uart0(addr) && dev_uart_present()) {
        dev_uart_write_reg(addr, v);
        return;
    }
    if (in_disk0(addr) && dev_disk0_present()) {
        disk0_wr32(addr, v);
        return;
    }
    ram_write32(addr, v);
}

// ---------- Bulk copy helpers ----------
bool mem_copy_in(uint32_t dst_addr, const void *src, size_t len) {
    if (!len) return true;
    if (!src) return false;

    // Disallow writing into MMIO windows via bulk copy
    if (in_disk0(dst_addr) || (len && in_disk0(dst_addr + (uint32_t)(len - 1))) ||
        in_uart0(dst_addr) || (len && in_uart0(dst_addr + (uint32_t)(len - 1)))) {
        return false;
    }

    if (!ram_ok(dst_addr, len)) return false;
    memcpy(g_ram_base + dst_addr, src, len);
    return true;
}

bool mem_copy_out(void *dst, uint32_t src_addr, size_t len) {
    if (!len) return true;
    if (!dst) return false;

    // Disallow reading from MMIO via bulk copy
    if (in_disk0(src_addr) || (len && in_disk0(src_addr + (uint32_t)(len - 1))) ||
        in_uart0(src_addr) || (len && in_uart0(src_addr + (uint32_t)(len - 1)))) {
        return false;
    }

    if (!ram_ok(src_addr, len)) return false;
    memcpy(dst, g_ram_base + src_addr, len);
    return true;
}
