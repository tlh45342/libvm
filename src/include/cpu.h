#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "debug.h"   // debug_flags_t + DBG_*

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HALT_NONE     = 0,
    HALT_DEADBEEF = 1,   // your convenience trap
    HALT_BKPT     = 2,   // breakpoint
    HALT_SWI      = 3,   // software interrupt (if you model it)
    HALT_UNDEF    = 4,   // undefined instruction
    HALT_ABORT    = 5    // data/prefetch abort, etc., if you add later
} halt_reason_t;

void cpu_halt(void);         // core requests halt
void cpu_clear_halt(void);   // clear halt (on reset)
bool cpu_is_halted(void);    // query halt flag

// ---------------- CPU state ----------------
typedef struct {
    uint32_t r[16];      // R0..R15 (R15=PC)
    uint32_t cpsr;       // Current PSR
    uint32_t spsr;       // Saved PSR (if you use it for mode changes)
    bool          halted;       // VM is stopped at a trap/breakpoint/etc
    halt_reason_t halt_reason;  // why it stopped
    uint32_t npc;     // next PC (fall-through or branch target)
    // banked SVC (if you use them)
    uint32_t spsr_svc;
    uint32_t lr_svc;
    uint32_t sp_svc;
} CPU;

extern CPU cpu;          // single global CPU instance (for now)
extern uint64_t cycle;

uint32_t arm_read_src_reg(int r);

// ---------------- Public configuration ----------------
#ifndef RAM_SIZE
#define RAM_SIZE (512u * 1024u * 1024u)  // 512 MiB default
#endif

#ifndef ENTRY_POINT
#define ENTRY_POINT 0x8000u
#endif

#ifndef BIT
#define BIT(x) (1u << (x))
#endif

// CPSR bit masks
#define CPSR_N   BIT(31)
#define CPSR_Z   BIT(30)
#define CPSR_C   BIT(29)
#define CPSR_V   BIT(28)
#define CPSR_Q   BIT(27)
#define CPSR_GE0 BIT(16)
#define CPSR_GE1 BIT(17)
#define CPSR_GE2 BIT(18)
#define CPSR_GE3 BIT(19)
#define CPSR_GE_MASK (BIT(16)|BIT(17)|BIT(18)|BIT(19))
#define CPSR_E   BIT(9)
#define CPSR_A   BIT(8)
#define CPSR_I   BIT(7)
#define CPSR_F   BIT(6)
#define CPSR_T   BIT(5)
#define CPSR_MODE_MASK 0x1Fu

// ---------------- Global run-state (temporary; will move into VM) ----------------
extern uint64_t    cycle;       // global cycle counter
extern bool        cpu_halted;  // run/stop latch

extern debug_flags_t debug_flags;  // <-- fixed type
extern bool          trace_all;

// ---------------- CPU API ----------------
// Execute *one* instruction that’s already been fetched/decoded in your flow.
bool     execute(uint32_t instr);

// Centralized fetch (PC bounds check + PC+=4)
uint32_t cpu_fetch(void);

// Backtrace API (owned by cpu.c)
#ifndef CPU_MAX_BACKTRACE
#define CPU_MAX_BACKTRACE 64
#endif
void     cpu_bt_push(uint32_t lr);
void     cpu_bt_pop(void);
int      cpu_bt_depth(void);
uint32_t cpu_bt_frame(int i);
void     cpu_dump_backtrace(void);

// Also expose the register dumper
void cpu_dump_registers(void);

void cpu_halt(void);         // request a halt from inside the CPU core
void cpu_clear_halt(void);   // clear halt flag (on reset)
bool cpu_is_halted(void);    // query halt state
void cpu_step(void);          // <-- add this prototype

void cpu_exception_return(uint32_t new_pc);

// (Optional compatibility: if other files still call dump_registers())
void dump_registers(void);  // provide wrapper in cpu.c
#ifdef __cplusplus
} // extern "C"
#endif

#endif // CPU_H