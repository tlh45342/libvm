// src/include/vm.h

#ifndef VM_H
#define VM_H
/* #pragma once */             /* Optional: fine to add, but not required with the guard */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "debug.h"             // defines debug_flags_t
typedef debug_flags_t vm_debug_t;  // optional alias; keep if you like the name

// Opaque type
typedef struct VM VM;

/* existing declarations … */

bool vm_load_binary(VM* vm, const char* path, uint32_t addr);  // <-- add
void vm_dump_regs(VM* vm);                                     // <-- add
// Memory accessors used by CLI 'e'
uint8_t vm_read8(VM *vm, uint32_t addr);

// Also add this one if you call it from cli.c:
void vm_set_debug(VM *vm, debug_flags_t flags);

// ---- Lifecycle ----
VM*           vm_create(void);                // Create VM w/o RAM (attach later)
bool          vm_add_ram(VM* vm, size_t ram_size);  // Attach/allocate RAM
size_t        vm_ram_size(const VM* vm);      // Query RAM size
void          vm_reset(VM* vm);               // Reset CPU & state
void          vm_destroy(VM* vm);             // Free VM and RAM
void          vm_set_debug(VM *vm, debug_flags_t flags);
debug_flags_t vm_get_debug(const VM *vm);

// ---- Execution ----
bool    vm_step(VM* vm);                // Execute one instruction
bool    vm_run(VM* vm, uint64_t max_cycles);   // 0 = run until halt
void    vm_halt(VM* vm);
bool    vm_is_halted(const VM* vm);

// ---- Memory convenience ----
bool    vm_load_image(VM* vm, const void* data, size_t len, uint32_t addr);
bool    vm_read_mem(VM* vm, uint32_t addr, void* out, size_t len);
bool    vm_write_mem(VM* vm, uint32_t addr, const void* in, size_t len);

// ---- Registers ----
uint32_t vm_get_reg(const VM* vm, int idx);    // 0..15
void     vm_set_reg(VM* vm, int idx, uint32_t value);
uint32_t vm_get_cpsr(const VM* vm);
void     vm_set_cpsr(VM* vm, uint32_t value);
void     vm_dump_regs(VM *vm);

// ---- MMIO callback registration ----
typedef uint32_t (*vm_mmio_read_fn)(void* ctx, uint32_t addr);
typedef void     (*vm_mmio_write_fn)(void* ctx, uint32_t addr, uint32_t value);

bool vm_map_mmio(VM* vm,
                 uint32_t base, uint32_t size,
                 vm_mmio_read_fn rfn, vm_mmio_write_fn wfn,
                 void* ctx);

#endif // VM_H
