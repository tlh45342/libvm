// src/include/debug.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Bitmask type for debug flags
typedef uint32_t debug_flags_t;

// Common flags (extend as you like)
enum {
    DBG_NONE      = 0,
    DBG_INSTR     = 1u << 0,  // instruction trace
    DBG_MEM_READ  = 1u << 1,
    DBG_MEM_WRITE = 1u << 2,
    DBG_MMIO      = 1u << 3,
    DBG_DISK      = 1u << 4,
    DBG_IRQ       = 1u << 5,
    DBG_DISASM    = 1u << 6,
	DBG_K12       = 1u << 7,   // NEW: K12 decode trace (aka KEY12)
	DBG_TRACE     = 1u << 8,
	DBG_CLI       = 1u << 9,
};

#define DBG_ALL (DBG_INSTR|DBG_MEM_READ|DBG_MEM_WRITE|DBG_MMIO|DBG_DISK|DBG_IRQ|DBG_DISASM|DBG_K12|DBG_TRACE|DBG_CLI)

// These are defined in some .c (e.g., cpu.c for debug_flags, or elsewhere)
extern debug_flags_t debug_flags;
extern bool          trace_all;
