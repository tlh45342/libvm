// src/vm.c

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>   // calloc, free
#include <errno.h>
#include <inttypes.h>

#include "vm.h"
#include "cpu.h"
#include "board.h"       // DTB_ADDR
#include "dtb_blob.h"
#include "mem.h"
#include "disasm.h"
#include "log.h"
#include "debug.h"
#include "hw_bus.h"
#include "dev_rtc.h"     // RTC mapping helpers
#include "dev_nvram.h"   // NVRAM mapping helpers

typedef struct VM {
    CPU         cpu;
    uint8_t    *ram;        // may be NULL until vm_add_ram()
    size_t      ram_size;   // 0 until vm_add_ram()
    uint64_t    cycle;
    bool        halted;
    debug_flags_t debug;

    struct {
        uint32_t base, size;
        vm_mmio_read_fn  rfn;
        vm_mmio_write_fn wfn;
        void    *ctx;
    } mmio[16];
    int n_mmio;
	bool devices_inited;    // <-- add this line
} VM;

// --------- forward declarations ----------

// --- device/DTB setup helpers (local to vm.c) ---
static void vm_map_rtc(void);
static void vm_map_nvram(void);
static void vm_place_dtb(struct VM* vm);
static void vm_init_devices_and_boot(struct VM* vm);

// TEMP: ensure prototype is visible even if hw_bus.h is stale
bool hw_bus_map_region(const char* name,
                       uint32_t base,
                       uint32_t size,
                       uint32_t (*read32)(uint32_t addr),
                       void (*write32)(uint32_t addr, uint32_t value));


// ---------- helper utils? ----------

// Little-endian 32-bit fetch using mem_* (keeps CPU core pure)
static inline uint32_t vm_read_le32(uint32_t addr) {
    uint32_t v =  (uint32_t)mem_read8(addr + 0)
                | ((uint32_t)mem_read8(addr + 1) << 8)
                | ((uint32_t)mem_read8(addr + 2) << 16)
                | ((uint32_t)mem_read8(addr + 3) << 24);
    return v;
}

// Minimal objdump-style line: "    8014:       e3e05000"
static inline void vm_trace_objdump_line(uint32_t pc, uint32_t instr) {
    // Match gnu objdump spacing for addr/word (lowercase hex)
    log_printf("    %04x:       %08x\n", (pc & 0xFFFF), instr);
}

// ---- lifecycle ----
// Create a VM with no RAM yet. Call vm_add_ram() next.
VM* vm_create(void) {
    VM *vm = (VM*)calloc(1, sizeof(VM));
    if (!vm) return NULL;
    vm_reset(vm);
    return vm;
}

// Attach/allocate RAM later. Returns false if RAM already attached.
bool vm_add_ram(VM* vm, size_t ram_size) {
    if (!vm || ram_size == 0) return false;
    if (vm->ram) {
        log_printf("[ERROR] vm_add_ram: RAM already attached (%zu bytes)\n", vm->ram_size);
        return false;
    }
    vm->ram = (uint8_t*)calloc(1, ram_size);
    if (!vm->ram) {
        log_printf("[ERROR] vm_add_ram: allocation failed for %zu bytes\n", ram_size);
        return false;
    }
    vm->ram_size = ram_size;

    // Bridge to current mem.c singleton:
	mem_init();                        // initialize memory subsystem once
	mem_bind(vm->ram, vm->ram_size);   // then bind the VM's RAM buffer

	if (!vm->devices_inited) {
		vm_init_devices_and_boot(vm);
    }
	
    // If reset ran before RAM existed, set SP now.
    if (vm->cpu.r[13] == 0) {
        vm->cpu.r[13] = (uint32_t)(vm->ram_size - 4); // SP
    }
    return true;
}

size_t vm_ram_size(const VM* vm) { return vm ? vm->ram_size : 0; }

void vm_destroy(VM* vm) {
    if (!vm) return;
    free(vm->ram);
    free(vm);
}

// vm_reset:
void vm_reset(VM* vm) {
    memset(&vm->cpu, 0, sizeof(vm->cpu));
    if (vm->ram && vm->ram_size) vm->cpu.r[13] = (uint32_t)(vm->ram_size - 4);
    else                         vm->cpu.r[13] = 0;
    vm->cpu.r[15] = ENTRY_POINT;
    vm->cycle = 0;
    vm->halted = false;
    cpu_clear_halt();              // <-- clear old halts in the core
}

// ---- run control ----
static inline bool vm_require_ram(VM* vm, const char* where) {
    if (vm->ram) return true;
	log_printf("[ERROR] %s: no RAM attached. Call vm_add_ram() first.\n", where);
    vm->halted = true;
    return false;
}

// vm_step:
bool vm_step(VM* vm)
{
    if (!vm || !vm->ram || vm->ram_size == 0) return false;

    mem_bind(vm->ram, vm->ram_size);
    cpu = vm->cpu;

    if (vm->debug & DBG_DISASM) {
        uint32_t pc    = cpu.r[15];
        uint32_t instr = vm_read_le32(pc);
        char buf[128];
        disasm_line(pc, instr, buf, sizeof(buf));
		log_printf("%08X:       %08X        %s\n", pc, instr, buf);
    }

    cpu_step();
    vm->cycle++;

    vm->cpu = cpu;
    return !cpu_is_halted();
}

bool vm_run(VM* vm, uint64_t max_cycles)
{
    if (!vm || !vm->ram || vm->ram_size == 0) return false;

    mem_bind(vm->ram, vm->ram_size);
    cpu = vm->cpu;

    uint64_t c = 0;
    while (!cpu_is_halted() && (max_cycles == 0 || c < max_cycles)) {

        if (vm->debug & DBG_DISASM) {
			uint32_t pc    = cpu.r[15];
			uint32_t instr = vm_read_le32(pc);
			char buf[128];
			disasm_line(pc, instr, buf, sizeof(buf));
			log_printf("%08X:       %08X        %s\n", pc, instr, buf);
        }

        cpu_step();
        c++;
    }

    vm->cpu = cpu;
    vm->cycle += c;
    return !cpu_is_halted();
}

void vm_halt(VM* vm) { vm->halted = true; }
bool vm_is_halted(const VM* vm) { return vm->halted; }

// ---- memory ----
bool vm_load_image(VM* vm, const void* data, size_t len, uint32_t addr) {
    if (!vm || !data) return false;
    if (!vm_require_ram(vm, "vm_load_image")) return false;
    if (addr > vm->ram_size || len > vm->ram_size - addr) return false;
    memcpy(vm->ram + addr, data, len);
    return true;
}

bool vm_read_mem(VM* vm, uint32_t addr, void* out, size_t len) {
    if (!vm || !out) return false;
    if (!vm_require_ram(vm, "vm_read_mem")) return false;
    uint8_t *p = (uint8_t*)out;
    for (size_t i = 0; i < len; i++) p[i] = mem_read8(addr + (uint32_t)i);
    return true;
}

bool vm_write_mem(VM* vm, uint32_t addr, const void* in, size_t len) {
    if (!vm || !in) return false;
    if (!vm_require_ram(vm, "vm_write_mem")) return false;
    const uint8_t *p = (const uint8_t*)in;
    for (size_t i = 0; i < len; i++) mem_write8(addr + (uint32_t)i, p[i]);
    return true;
}

// ---- registers ----
uint32_t vm_get_reg(const VM* vm, int idx) {
    if (!vm || idx < 0 || idx > 15) return 0;
    return vm->cpu.r[idx];
}
void vm_set_reg(VM* vm, int idx, uint32_t value) {
    if (!vm || idx < 0 || idx > 15) return;
    vm->cpu.r[idx] = value;
}
uint32_t vm_get_cpsr(const VM* vm) { return vm ? vm->cpu.cpsr : 0; }
void     vm_set_cpsr(VM* vm, uint32_t v) { if (vm) vm->cpu.cpsr = v; }

// ---- MMIO registry ----
bool vm_map_mmio(VM* vm, uint32_t base, uint32_t size,
                 vm_mmio_read_fn rfn, vm_mmio_write_fn wfn, void* ctx) {
    if (!vm || vm->n_mmio >= (int)(sizeof vm->mmio / sizeof vm->mmio[0])) return false;
    vm->mmio[vm->n_mmio++] = (typeof(vm->mmio[0])){ base, size, rfn, wfn, ctx };
    return true;
}

// If your vm.h prototype is: int vm_load_binary(VM *vm, const char *path, uint32_t addr);
// keep this exact signature. If it's (VM*, const char*), remove the addr param and
// use LOAD_DEFAULT_ADDR instead.


bool vm_load_binary(VM* vm, const char* path, uint32_t addr) {
    if (!vm || !path) return false;
    if (!vm->ram) { log_printf("Load failed: VM RAM not initialized\n"); return false; }
    if (addr >= vm->ram_size) {
        log_printf("Load failed: addr 0x%08X beyond RAM size %zu\n", addr, vm->ram_size);
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        log_printf("Failed to open '%s': %s\n", path, strerror(errno));
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return false; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }

    size_t size = (size_t)sz;
    if (size == 0) {
        fclose(f);
        log_printf("Load skipped: '%s' is empty\n", path);
        return false;
    }

    // Bounds check (use 64-bit to avoid wrap)
    if ((uint64_t)addr + (uint64_t)size > (uint64_t)vm->ram_size) {
        fclose(f);
        log_printf("Load overflow: addr=0x%08X size=%zu ram=%zu\n", addr, size, vm->ram_size);
        return false;
    }

    size_t n = fread(vm->ram + addr, 1, size, f);
    fclose(f);
    if (n != size) {
        log_printf("Short read loading '%s' (got %zu of %zu)\n", path, n, size);
        return false;
    }

    log_printf("[LOAD] %s @ 0x%08X (%zu bytes)\n", path, addr, size);
    return true;
}

// If your prototype is: void vm_dump_regs(const VM *vm);
// keep it. If it's VM* (non-const), just drop the const.
void vm_dump_regs(VM* vm) {
    if (!vm) return;
    const CPU *c = &vm->cpu;
    for (int i = 0; i < 16; ++i) {
        log_printf("r%-2d = 0x%08X%s", i, c->r[i], (i % 4 == 3) ? "\n" : "  ");
    }
    log_printf("CPSR = 0x%08X  cycle=%" PRIu64 "\n", c->cpsr, vm->cycle);
}

void vm_set_debug(VM *vm, debug_flags_t flags) {
    if (!vm) return;
    vm->debug = flags;
    debug_flags = flags;  // keep CPU/handlers in sync
    // log_printf("[DEBUG] debug_flags set to 0x%08X\n", flags); // optional
}

void vm_clear_halt(VM *vm) {
    (void)vm;          // if you don’t need it yet, silence unused
    cpu_clear_halt();  // delegate to CPU
}

static void vm_map_rtc(void) {
    // Initialize the device state
    dev_rtc_init(RTC_BASE_ADDR);

    // Expose it on the MMIO bus
    hw_bus_map_region("rtc",
                      RTC_BASE_ADDR,
                      RTC_MMIO_SIZE,
                      /*read32=*/dev_rtc_read32,
                      /*write32=*/dev_rtc_write32);
}

static void vm_place_dtb(struct VM* vm) {
    if (dtb_blob_len == 0) return;

    uint32_t dst = DTB_ADDR;

    // Copy DTB blob into guest memory
    size_t i = 0;
    for (; i + 4 <= dtb_blob_len; i += 4, dst += 4) {
        uint32_t w =  (uint32_t)dtb_blob[i]
                    | ((uint32_t)dtb_blob[i+1] << 8)
                    | ((uint32_t)dtb_blob[i+2] << 16)
                    | ((uint32_t)dtb_blob[i+3] << 24);
        mem_write32(dst, w);
    }
    if (i < dtb_blob_len) {
        // tail (<4 bytes)
        uint32_t w = mem_read32(dst);
        uint32_t j = 0;
        while (i + j < dtb_blob_len) {
            uint32_t b = (uint32_t)dtb_blob[i + j];
            w = (w & ~(0xFFu << (8*j))) | (b << (8*j));
            ++j;
        }
        mem_write32(dst, w);
    }

    // Linux/ATAGS-style boot args (r0=0, r1=arch, r2=DTB addr).
    // If you don't have an arch ID, keep r1=0.
    vm->cpu.r[0] = 0;
    vm->cpu.r[1] = 0;
    vm->cpu.r[2] = DTB_ADDR;
}

// Call once during startup, after RAM is ready and before you start executing:
static void vm_init_devices_and_boot(struct VM* vm) {
    static bool g_devices_inited = false;
    if (!g_devices_inited) {
        vm_map_rtc();
        vm_map_nvram();
        g_devices_inited = true;
    }
    // Place (or refresh) the DTB image each time RAM is (re)bound,
    // so guest always sees a valid blob at DTB_ADDR.
    vm_place_dtb(vm);
}

static void vm_map_nvram(void) {
    dev_nvram_init("nvram.bin");
    hw_bus_map_region("nvram", NVRAM_BASE_ADDR, NVRAM_MMIO_SIZE,
                      /*read32=*/dev_nvram_read32,
                      /*write32=*/dev_nvram_write32);
}

