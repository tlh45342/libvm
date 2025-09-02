// memops.c (NPC-aware)
// - Address reads use arm_read_src_reg() so Rn/Rm==15 read as PC+8 (ARM).
// - Any load that writes PC sets cpu.npc instead of writing cpu.r[15] directly.

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "cpu.h"      // CPU + extern CPU cpu, debug_flags, trace_all, arm_read_src_reg
#include "mem.h"      // mem_read8/32, mem_write8/32
#include "log.h"      // log_printf
#include "memops.h"   // prototypes
#include "debug.h"
#include "cond.h"     // for evaluate_condition()
#include "operand.h"

// ---------- helpers ----------
static inline uint32_t addr_read_reg(int r) {
    return arm_read_src_reg(r); // PC as source => PC+8 in ARM state
}
static inline void write_pc_via_npc(uint32_t new_pc) {
    cpu.npc = new_pc & ~3u;     // keep ARM aligned
}

// Compute effective address for extra load/store (halfword/signed family).
static inline uint32_t extra_addr(uint32_t instr,
                                  uint32_t base,
                                  uint32_t *new_base_out,
                                  bool *do_wb_out) {
    uint32_t P = (instr >> 24) & 1u;   // pre/post
    uint32_t U = (instr >> 23) & 1u;   // up/down
    uint32_t I = (instr >> 22) & 1u;   // imm/reg
    uint32_t W = (instr >> 21) & 1u;   // writeback

    // imm8 = bits [11:8]|[3:0]
    uint32_t imm8 = ((instr >> 4) & 0xF0u) | (instr & 0x0Fu);
    uint32_t off  = I ? imm8 : addr_read_reg(instr & 0xFu); // Rm as *source* (PC=>PC+8)
    int32_t  delta = U ? (int32_t)off : -(int32_t)off;

    uint32_t ea       = P ? (uint32_t)((int32_t)base + delta) : base; // effective address
    uint32_t new_base = (uint32_t)((int32_t)base + delta);            // base after update

    if (new_base_out) *new_base_out = new_base;
    if (do_wb_out)    *do_wb_out    = (bool)(W || !P); // post-index => always wb

    return ea;
}

// -----------------------------------------------------------------------------
// STR / LDR byte/word, LDM/STM, POP/POP{..,pc}, LDR(literal), LDRD/STRD
// -----------------------------------------------------------------------------

void handle_str_postimm(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint32_t imm = instr & 0xFFF;

    uint32_t addr = addr_read_reg(rn);
    uint32_t val  = cpu.r[rd];

    mem_write32(addr, val);
    cpu.r[rn] = addr + imm;

    if (debug_flags & DBG_INSTR)
        log_printf("  [STR post-imm] mem[0x%08X] <= r%d (0x%08X); r%d += 0x%X\n",
                   addr, rd, val, rn, imm);
}

void handle_ldrb_reg(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;

    uint32_t addr = addr_read_reg(rn);
    uint8_t  byte = mem_read8(addr);
    cpu.r[rd] = byte; // LDRB into PC is UNPREDICTABLE; we keep it simple.

    if (trace_all || (debug_flags & DBG_MEM_READ))
        log_printf("[LDRB reg] r%d = mem8[0x%08X] => 0x%02X\n", rd, addr, byte);
}

void handle_strb_preimm(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint32_t offset = instr & 0xFFF;
    bool up  = (instr >> 23) & 1u;
    bool pre = (instr >> 24) & 1u;

    uint32_t base = addr_read_reg(rn);
    uint32_t addr = pre ? (up ? base + offset : base - offset) : base;

    uint8_t val = (uint8_t)(cpu.r[rd] & 0xFFu);
    mem_write8(addr, val);

    if (debug_flags & DBG_INSTR)
        log_printf("[STRB pre-%s imm] mem[0x%08X] <= r%d (0x%02X)\n",
                   up ? "inc" : "dec", addr, rd, val);
}

// STM
void handle_stm(uint32_t instr) {
    uint8_t  Rn  = (instr >> 16) & 0xFu;
    uint32_t list=  instr        & 0xFFFFu;
    uint32_t P   = (instr >> 24) & 1u;
    uint32_t U   = (instr >> 23) & 1u;
    uint32_t W   = (instr >> 21) & 1u;

    uint32_t base = addr_read_reg(Rn);                 // PC as source => PC+8
    uint32_t addr = base;

    for (uint32_t r = 0; r <= 15; ++r) {
        if ((list >> r) & 1u) {
            if (P) addr += U ? 4u : (uint32_t)-4;      // pre-index
            mem_write32(addr, cpu.r[r]);
            if (!P) addr += U ? 4u : (uint32_t)-4;     // post-index
        }
    }
    if (W) cpu.r[Rn] = addr;                            // writeback
}

void handle_strb_postimm(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint32_t offset = instr & 0xFFF;

    uint32_t addr = addr_read_reg(rn);
    uint8_t  byte = (uint8_t)(cpu.r[rd] & 0xFFu);
    mem_write8(addr, byte);
    cpu.r[rn] = addr + offset;

    if (debug_flags & DBG_INSTR)
        log_printf("[STRB post-imm] mem[0x%08X] <= r%d (0x%02X); r%d += 0x%X\n",
                   addr, rd, byte, rn, offset);
}

void handle_str_preimm(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint32_t offset = instr & 0xFFF;
    bool up  = (instr >> 23) & 1u;
    bool pre = (instr >> 24) & 1u;

    uint32_t base = addr_read_reg(rn);
    uint32_t addr = pre ? (up ? base + offset : base - offset) : base;

    mem_write32(addr, cpu.r[rd]);

    if (debug_flags & DBG_INSTR)
        log_printf("[STR pre-%s imm] mem[0x%08X] <= r%d (0x%08X)\n",
                   up ? "inc" : "dec", addr, rd, cpu.r[rd]);
}

void handle_str_predec(uint32_t instr) {
    int rd = (instr >> 12) & 0xF;
    int rn = (instr >> 16) & 0xF;
    uint32_t offset = instr & 0xFFF;

    uint32_t base = addr_read_reg(rn);
    uint32_t addr = base - offset;

    mem_write32(addr, cpu.r[rd]);
    cpu.r[rn] = addr;

    if (debug_flags & DBG_INSTR)
        log_printf("[STR pre-dec] mem[0x%08X] <= r%d (0x%08X)\n", addr, rd, cpu.r[rd]);
}

void handle_ldr_literal(uint32_t instr) {
    // Rn==PC, P=1, I=0 : addr = Align4(PC)+8 +/- imm12
    uint32_t pc    = cpu.r[15];
    uint32_t imm12 = instr & 0xFFFu;
    uint32_t base  = (pc & ~3u) + 8u;
    bool U         = (instr >> 23) & 1u;
    uint32_t addr  = U ? (base + imm12) : (base - imm12);

    uint32_t rd = (instr >> 12) & 0xFu;
    uint32_t val = mem_read32(addr);

    if (rd == 15) {
        write_pc_via_npc(val);
        if (debug_flags & DBG_INSTR)
            log_printf("[LDR lit] pc <= [0x%08X] => 0x%08X (npc)\n", addr, val);
    } else {
        cpu.r[rd] = val;
        if (debug_flags & DBG_INSTR)
            log_printf("[LDR lit] r%u <= [0x%08X] => 0x%08X\n", rd, addr, val);
    }
}

void handle_ldr_preimm(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint32_t imm = instr & 0xFFF;
    bool U = (instr >> 23) & 1u;

    uint32_t base = addr_read_reg(rn);
    uint32_t addr = U ? (base + imm) : (base - imm);

    uint32_t val = mem_read32(addr);
    if (rd == 15) {
        write_pc_via_npc(val);
        if (debug_flags & DBG_INSTR)
            log_printf("[LDR pre-%s imm] pc <= mem[0x%08X] => 0x%08X (npc)\n",
                       U?"inc":"dec", addr, val);
    } else {
        cpu.r[rd] = val;
        if (debug_flags & DBG_INSTR)
            log_printf("[LDR pre-%s imm] r%d = mem[0x%08X] => 0x%08X\n",
                       U?"inc":"dec", rd, addr, val);
    }
}

void handle_ldr_postimm(uint32_t instr) {
    uint8_t rd = (instr >> 12) & 0xF;
    uint8_t rn = (instr >> 16) & 0xF;
    uint32_t imm = instr & 0xFFF;
    uint32_t U   = (instr >> 23) & 1u;

    uint32_t base = addr_read_reg(rn);
    uint32_t addr = base;
    uint32_t val  = mem_read32(addr);

    if (rd == 15) {
        write_pc_via_npc(val);
        if (trace_all || (debug_flags & DBG_MEM_READ))
            log_printf("[LDR post-imm] pc = mem[0x%08X] => 0x%08X (npc); r%d %c= 0x%X\n",
                       addr, val, rn, U?'+':'-', imm);
    } else {
        cpu.r[rd] = val;
        if (trace_all || (debug_flags & DBG_MEM_READ))
            log_printf("[LDR post-imm] r%d = mem[0x%08X] => 0x%08X; r%d %c= 0x%X\n",
                       rd, addr, val, rn, U?'+':'-', imm);
    }
    cpu.r[rn] = U ? (base + imm) : (base - imm);
}

void handle_ldrb_preimm(uint32_t instr) {
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint32_t offset = instr & 0xFFF;
    bool up  = (instr >> 23) & 1u;
    bool pre = (instr >> 24) & 1u;
    bool wb  = (instr >> 21) & 1u;

    uint32_t base = addr_read_reg(rn);
    uint32_t addr = pre ? (up ? base + offset : base - offset) : base;

    uint8_t val = mem_read8(addr);
    cpu.r[rd] = val;

    if (wb || !pre) cpu.r[rn] = addr;

    if (debug_flags & DBG_INSTR)
        log_printf("[LDRB pre-%s%s imm] r%d = mem8[0x%08X] => 0x%02X\n",
                   up ? "inc" : "dec", wb ? "!" : "", rd, addr, val);
}

void handle_ldrb_postimm(uint32_t instr) {
    uint32_t rn   = (instr >> 16) & 0xF;
    uint32_t rd   = (instr >> 12) & 0xF;
    uint32_t off  =  instr        & 0xFFF;

    uint32_t base = addr_read_reg(rn);
    uint8_t  val  = mem_read8(base);

    cpu.r[rd]  = val;
    cpu.r[rn]  = base + off;

    if (trace_all || (debug_flags & DBG_MEM_READ)) {
        log_printf("[LDRB post-imm] r%d = mem8[0x%08X] => 0x%02X; r%d += 0x%X\n",
                   rd, base, val, rn, off);
    }
}

void handle_pop(uint32_t instr) {
    uint32_t reglist = instr & 0xFFFF;
    uint32_t addr    = cpu.r[13];        // SP raw is fine for writeback

    size_t msz = mem_size();
    if (!mem_is_bound() || msz < 4) {
        log_printf("  [ERROR] No memory bound while POP\n");
        return;
    }

    for (int i = 0; i < 16; i++) {
        if (reglist & (1u << i)) {
            uint32_t val = mem_read32(addr);

            if (i == 15) {
                write_pc_via_npc(val);
            } else {
                cpu.r[i] = val;
            }
            if (debug_flags & DBG_MEM_READ)
                log_printf("[POP] r%d%s <= mem[0x%08X] => 0x%08X\n",
                           i, (i==15?"(pc via npc)":""), addr, val);
            addr += 4;
        }
    }
    cpu.r[13] = addr;
}

void handle_pop_pc(uint32_t instr) {
    (void)instr; // imm in old version wasn't architectural POP; ignore.

    size_t msz = mem_size();
    if (!mem_is_bound() || msz < 4) {
        log_printf("  [ERROR] No memory bound while POP {..,pc}\n");
        return;
    }

    uint32_t ret = mem_read32(cpu.r[13]);
    write_pc_via_npc(ret);
    cpu.r[13] += 4;

    if (debug_flags & DBG_INSTR)
        log_printf("  [POP PC] npc<=0x%08X, SP=0x%08X\n", cpu.npc, cpu.r[13]);
}

// ---- LDRD / STRD -----------------------------------------------------------

#ifndef GET_BITS
#define GET_BITS(x,hi,lo) (((x) >> (lo)) & ((1u << ((hi)-(lo)+1)) - 1))
#endif
#ifndef GET_BIT
#define GET_BIT(x,b) (((x) >> (b)) & 1u)
#endif

static inline uint32_t ldrd_strd_imm_off(uint32_t instr) {
    uint32_t imm4H = GET_BITS(instr, 11, 8);
    uint32_t imm4L = GET_BITS(instr,  3, 0);
    return (imm4H << 4) | imm4L;
}

static bool ldrd_strd_check(uint32_t Rn, uint32_t Rt, bool writeback, const char *mnemonic) {
    // Rt must be even; neither Rt nor Rt+1 may be r15
    if ((Rt & 1u) != 0u || Rt == 15u || (Rt + 1u) == 15u) {
        log_printf("[ERROR] %s: Rt must be even and Rt/Rt+1 must not be r15 (Rt=%u)\n",
                   mnemonic, Rt);
        return false;
    }
    // Writeback may not target a base that overlaps the destination pair
    if (writeback && (Rn == Rt || Rn == (Rt + 1u))) {
        log_printf("[ERROR] %s: writeback hazard: Rn==Rt or Rn==Rt+1 (Rn=%u, Rt=%u)\n",
                   mnemonic, Rn, Rt);
        return false;
    }
    return true;
}

static bool do_ldrd(uint32_t addr, uint32_t Rt) {
    if (addr & 3u) {
        log_printf("[ERROR] LDRD unaligned address 0x%08X\n", addr);
        return false;
    }
    uint32_t lo = mem_read32(addr);
    uint32_t hi = mem_read32(addr + 4);
    cpu.r[Rt]     = lo;
    cpu.r[Rt + 1] = hi;

    if (trace_all || (debug_flags & DBG_MEM_READ)) {
        log_printf("[LDRD] r%u=0x%08X r%u=0x%08X from [0x%08X]\n",
                   Rt, lo, Rt + 1, hi, addr);
    }
    return true;
}

static bool do_strd(uint32_t addr, uint32_t Rt) {
    if (addr & 3u) {
        log_printf("[ERROR] STRD unaligned address 0x%08X\n", addr);
        return false;
    }
    uint32_t lo = cpu.r[Rt];
    uint32_t hi = cpu.r[Rt + 1];
    mem_write32(addr,     lo);
    mem_write32(addr + 4, hi);

    if (trace_all || (debug_flags & DBG_MEM_WRITE)) {
        log_printf("[STRD] [0x%08X] <= r%u=0x%08X, [0x%08X] <= r%u=0x%08X\n",
                   addr, Rt, lo, addr + 4, Rt + 1, hi);
    }
    return true;
}

// Immediate form (I=1), bits[7:4]==0b1111
void exec_ldrd_imm(uint32_t instr) {
    uint32_t P = GET_BIT(instr, 24);
    uint32_t U = GET_BIT(instr, 23);
    uint32_t W = GET_BIT(instr, 21);
    uint32_t Rn = GET_BITS(instr, 19, 16);
    uint32_t Rt = GET_BITS(instr, 15, 12);

    bool writeback = W || !P;
    if (!ldrd_strd_check(Rn, Rt, writeback, "LDRD")) return;

    uint32_t base  = addr_read_reg(Rn);
    uint32_t off   = ldrd_strd_imm_off(instr);
    uint32_t delta = U ? off : (uint32_t)(-((int32_t)off));
    uint32_t addr  = P ? (base + delta) : base;

    if (do_ldrd(addr, Rt) && writeback) cpu.r[Rn] = base + delta;
}

// Register form (I=0), bits[7:4]==0b1111
void exec_ldrd_reg(uint32_t instr) {
    uint32_t P  = GET_BIT(instr, 24);
    uint32_t U  = GET_BIT(instr, 23);
    uint32_t W  = GET_BIT(instr, 21);
    uint32_t Rn = GET_BITS(instr, 19, 16);
    uint32_t Rt = GET_BITS(instr, 15, 12);
    uint32_t Rm = GET_BITS(instr,  3,  0);

    bool writeback = W || !P;
    if (!ldrd_strd_check(Rn, Rt, writeback, "LDRD")) return;

    uint32_t base  = addr_read_reg(Rn);
    uint32_t off   = addr_read_reg(Rm);
    uint32_t delta = U ? off : (uint32_t)(-((int32_t)off));
    uint32_t addr  = P ? (base + delta) : base;

    if (do_ldrd(addr, Rt) && writeback) cpu.r[Rn] = base + delta;
}

void exec_strd_imm(uint32_t instr) {
    uint32_t P  = GET_BIT(instr, 24);
    uint32_t U  = GET_BIT(instr, 23);
    uint32_t W  = GET_BIT(instr, 21);
    uint32_t Rn = GET_BITS(instr, 19, 16);
    uint32_t Rt = GET_BITS(instr, 15, 12);

    bool writeback = W || !P;
    if (!ldrd_strd_check(Rn, Rt, writeback, "STRD")) return;

    uint32_t base  = addr_read_reg(Rn);
    uint32_t off   = ldrd_strd_imm_off(instr);
    uint32_t delta = U ? off : (uint32_t)(-((int32_t)off));
    uint32_t addr  = P ? (base + delta) : base;

    if (do_strd(addr, Rt) && writeback) cpu.r[Rn] = base + delta;
}

void exec_strd_reg(uint32_t instr) {
    uint32_t P  = GET_BIT(instr, 24);
    uint32_t U  = GET_BIT(instr, 23);
    uint32_t W  = GET_BIT(instr, 21);
    uint32_t Rn = GET_BITS(instr, 19, 16);
    uint32_t Rt = GET_BITS(instr, 15, 12);
    uint32_t Rm = GET_BITS(instr,  3,  0);

    bool writeback = W || !P;
    if (!ldrd_strd_check(Rn, Rt, writeback, "STRD")) return;

    uint32_t base  = addr_read_reg(Rn);
    uint32_t off   = addr_read_reg(Rm);
    uint32_t delta = U ? off : (uint32_t)(-((int32_t)off));
    uint32_t addr  = P ? (base + delta) : base;

    if (do_strd(addr, Rt) && writeback) cpu.r[Rn] = base + delta;
}

// --- helpers near others in memops.c ---
static inline uint32_t am2_shift(uint32_t instr, uint32_t rmval) {
    // Addressing Mode 2 register offset shift (imm only, bit4==0)
    uint32_t stype  = (instr >> 5) & 0x3u;   // 0 LSL, 1 LSR, 2 ASR, 3 ROR
    uint32_t shimm  = (instr >> 7) & 0x1Fu;  // shift immediate
    switch (stype) {
        case 0: // LSL
            return (shimm == 0) ? rmval : (rmval << shimm);
        case 1: // LSR
            return (shimm == 0) ? 0 : (rmval >> shimm);
        case 2: // ASR
            return shimm ? ((uint32_t)((int32_t)rmval >> shimm))
                         : (rmval & 0x80000000u ? 0xFFFFFFFFu : 0);
        default: { // ROR (shimm==0 => RRX with CPSR C)
            if (shimm == 0) {
                uint32_t c = (cpu.cpsr >> 29) & 1u;
                return (rmval >> 1) | (c << 31);
            }
            shimm &= 31u;
            return (rmval >> shimm) | (rmval << (32u - shimm));
        }
    }
}

// LDR (word) — register offset addressing (AM2, bit25=1, B=0, L=1)
void handle_ldr_regoffset(uint32_t instr) {
    // Ensure this really is AM2 reg-offset (bit25=1) and bit4==0 (imm shift)
    if (((instr >> 25) & 1u) == 0) return;
    if (((instr >> 4) & 1u) != 0)  return;

    uint32_t P = (instr >> 24) & 1u;
    uint32_t U = (instr >> 23) & 1u;
    uint32_t W = (instr >> 21) & 1u;
    uint32_t L = (instr >> 20) & 1u;
    uint32_t B = (instr >> 22) & 1u;
    if (L != 1 || B != 0) return; // this handler is only LDR word

    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;

    uint32_t base   = addr_read_reg(Rn);
    uint32_t rmval  = addr_read_reg(Rm);
    uint32_t offset = am2_shift(instr, rmval);
    uint32_t delta  = U ? offset : (uint32_t)(-((int32_t)offset));
    uint32_t addr   = P ? (base + delta) : base;

    uint32_t val = mem_read32(addr);
    if (Rd == 15) {
        write_pc_via_npc(val);
    } else {
        cpu.r[Rd] = val;
    }

    // Writeback when W=1, or post-indexed form (P=0 implies writeback)
    if (W || !P) cpu.r[Rn] = base + delta;

    if (trace_all || (debug_flags & DBG_MEM_READ)) {
        log_printf("[LDR reg-off] r%u%s = mem32[0x%08X] => 0x%08X  (Rn=r%u%s, off=%c0x%X%s)\n",
                   Rd, (Rd==15?"(pc via npc)":""),
                   addr, val, Rn,
                   (W||!P)?" wb":"", U?'+':'-', offset,
                   P? " pre": " post");
    }
}

// Minimal AM2 shift for address calculation (carry ignored for addressing)
static inline uint32_t am2_shift_imm(uint32_t val, uint32_t stype, uint32_t sh_imm) {
    sh_imm &= 0x1F;
    switch (stype & 3u) {
        case 0: // LSL
            return (sh_imm ? (val << sh_imm) : val);
        case 1: // LSR
            return (sh_imm ? (val >> sh_imm) : 0);
        case 2: // ASR
            return (sh_imm ? ((uint32_t)((int32_t)val >> sh_imm)) : (val & 0x80000000u ? 0xFFFFFFFFu : 0));
        case 3: // ROR (sh_imm==0 -> RRX)
            if (sh_imm == 0) {
                uint32_t c = (cpu.cpsr >> 29) & 1u;
                return (val >> 1) | (c << 31);
            }
            return (val >> sh_imm) | (val << (32 - sh_imm));
    }
    return val;
}

void handle_ldrb_reg_shift(uint32_t instr) {
    // AM2: I=1, B=1, L=1, bit4==0 (imm shift)
    uint32_t rn    = (instr >> 16) & 0xF;
    uint32_t rd    = (instr >> 12) & 0xF;
    uint32_t rm    =  instr        & 0xF;

    uint32_t stype = (instr >> 5) & 0x3u;   // 0 LSL,1 LSR,2 ASR,3 ROR
    uint32_t sh_imm= (instr >> 7) & 0x1Fu;  // shift immediate
    uint32_t P     = (instr >> 24) & 1u;    // pre/post
    uint32_t U     = (instr >> 23) & 1u;    // add/sub
    uint32_t W     = (instr >> 21) & 1u;    // writeback

    uint32_t base   = addr_read_reg(rn);
    uint32_t off    = am2_shift_imm(addr_read_reg(rm), stype, sh_imm);
    uint32_t delta  = U ? off : (uint32_t)(-((int32_t)off));
    uint32_t addr   = P ? (base + delta) : base;        // pre: use updated, post: use base
    uint32_t new_rn = base + delta;                     // value for writeback when needed

    uint8_t  val = mem_read8(addr);
    cpu.r[rd] = (uint32_t)val;

    if (W || !P) cpu.r[rn] = new_rn;                    // writeback for W==1 or post-indexed

    if (trace_all || (debug_flags & DBG_MEM_READ)) {
        static const char* S[4] = {"LSL","LSR","ASR","ROR"};
        log_printf("[LDRB reg/shift] r%u = mem8[0x%08X] => 0x%02X  (r%d %c= %s #%u%s)\n",
                   rd, addr, val, rn, U?'+':'-', S[stype], sh_imm, (W||!P)?" wb":"");
    }
}

// STRH — store halfword (zero top 16 bits of data)
// STRH — store halfword (little-endian)
void handle_strh(uint32_t instr) {
    uint8_t  Rn = (instr >> 16) & 0xFu;
    uint8_t  Rd = (instr >> 12) & 0xFu;
    uint32_t base = cpu.r[Rn];

    uint32_t new_base; bool wb;
    uint32_t addr = extra_addr(instr, base, &new_base, &wb);

    uint16_t v = (uint16_t)(cpu.r[Rd] & 0xFFFFu);
    mem_write8(addr + 0, (uint8_t)(v & 0xFFu));
    mem_write8(addr + 1, (uint8_t)(v >> 8));

    if (wb) cpu.r[Rn] = new_base;
}

// LDRH — load halfword (zero-extend)
void handle_ldrh(uint32_t instr) {
    uint8_t  Rn = (instr >> 16) & 0xFu;
    uint8_t  Rd = (instr >> 12) & 0xFu;
    uint32_t base = cpu.r[Rn];

    uint32_t new_base; bool wb;
    uint32_t addr = extra_addr(instr, base, &new_base, &wb);

    uint16_t v = (uint16_t)((uint16_t)mem_read8(addr + 0)
                           | ((uint16_t)mem_read8(addr + 1) << 8));
    cpu.r[Rd] = (uint32_t)v;

    if (wb) cpu.r[Rn] = new_base;
}

// LDRSB — load signed byte
void handle_ldrsb(uint32_t instr) {
    uint8_t  Rn = (instr >> 16) & 0xFu;
    uint8_t  Rd = (instr >> 12) & 0xFu;
    uint32_t base = cpu.r[Rn];

    uint32_t new_base; bool wb;
    uint32_t addr = extra_addr(instr, base, &new_base, &wb);

    int8_t sb = (int8_t)mem_read8(addr);
    cpu.r[Rd] = (uint32_t)(int32_t)sb;

    if (wb) cpu.r[Rn] = new_base;
}

// LDRSH — load signed halfword
void handle_ldrsh(uint32_t instr) {
    uint8_t  Rn = (instr >> 16) & 0xFu;
    uint8_t  Rd = (instr >> 12) & 0xFu;
    uint32_t base = cpu.r[Rn];

    uint32_t new_base; bool wb;
    uint32_t addr = extra_addr(instr, base, &new_base, &wb);

    uint16_t raw = (uint16_t)((uint16_t)mem_read8(addr + 0)
                             | ((uint16_t)mem_read8(addr + 1) << 8));
    int16_t sh = (int16_t)raw;
    cpu.r[Rd] = (uint32_t)(int32_t)sh;

    if (wb) cpu.r[Rn] = new_base;
}

static inline uint32_t reglist_count(uint32_t list) {
    // builtin popcount if you prefer; this is portable
    uint32_t c = 0;
    while (list) { list &= (list - 1); ++c; }
    return c;
}

// Generic LDM/STM covering IA/IB/DA/DB, with P/U/W respected.
// S bit (user-mode banked or PSR restore) is TODO for now.
// LDM
void handle_ldm(uint32_t instr) {
    uint8_t  Rn  = (instr >> 16) & 0xFu;
    uint32_t list=  instr        & 0xFFFFu;
    uint32_t P   = (instr >> 24) & 1u;
    uint32_t U   = (instr >> 23) & 1u;
    uint32_t W   = (instr >> 21) & 1u;
    // uint32_t S = (instr >> 22) & 1u; // TODO

    uint32_t base = addr_read_reg(Rn);         // PC as source => PC+8
    uint32_t addr = base;

    for (uint32_t r = 0; r <= 15; ++r) {
        if ((list >> r) & 1u) {
            if (P) addr += U ? 4u : (uint32_t)-4;      // pre-index
            uint32_t val = mem_read32(addr);
            if (r == 15) {
                write_pc_via_npc(val);                 // npc for PC
            } else {
                cpu.r[r] = val;
            }
            if (!P) addr += U ? 4u : (uint32_t)-4;     // post-index
        }
    }
    if (W) cpu.r[Rn] = addr;                            // writeback
}

// ---- STR (word) — register offset addressing (AM2, bit25=1, B=0, L=0) ----
void handle_str_regoffset(uint32_t instr) {
    // Must be AM2 reg-offset (I=1) with imm shift (bit4==0), B=0, L=0
    if (((instr >> 25) & 1u) == 0) return;
    if (((instr >> 22) & 1u) != 0) return; // B must be 0 (word)
    if (((instr >> 20) & 1u) != 0) return; // L must be 0 (store)
    if (((instr >>  4) & 1u) != 0) return; // bit4 must be 0 (imm shift)

    uint32_t P = (instr >> 24) & 1u;
    uint32_t U = (instr >> 23) & 1u;
    uint32_t W = (instr >> 21) & 1u;

    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;

    uint32_t base   = addr_read_reg(Rn);
    uint32_t rmval  = addr_read_reg(Rm);
    uint32_t offset = am2_shift(instr, rmval);
    uint32_t delta  = U ? offset : (uint32_t)(-((int32_t)offset));
    uint32_t addr   = P ? (base + delta) : base;

    mem_write32(addr, cpu.r[Rd]);
    if (W || !P) cpu.r[Rn] = base + delta;

    if (debug_flags & DBG_MEM_WRITE) {
        log_printf("[STR reg-off] [0x%08X] <= r%u=0x%08X%s\n",
                   addr, Rd, cpu.r[Rd], (W||!P)?" wb":"");
    }
}

// ---- STRB (byte) — reg offset (imm shift) pre/post, W ----
void handle_strb_reg_shift(uint32_t instr) {
    // AM2: I=1, B=1, L=0, bit4==0 (imm shift)
    if (((instr >> 25) & 1u) == 0) return; // I=1
    if (((instr >> 22) & 1u) == 0) return; // B=1
    if (((instr >> 20) & 1u) != 0) return; // L=0
    if (((instr >>  4) & 1u) != 0) return; // imm shift form

    uint32_t rn    = (instr >> 16) & 0xFu;
    uint32_t rd    = (instr >> 12) & 0xFu;
    uint32_t rm    =  instr        & 0xFu;

    uint32_t P     = (instr >> 24) & 1u;
    uint32_t U     = (instr >> 23) & 1u;
    uint32_t W     = (instr >> 21) & 1u;

    uint32_t stype = (instr >> 5) & 0x3u;
    uint32_t shimm = (instr >> 7) & 0x1Fu;

    uint32_t base   = addr_read_reg(rn);
    uint32_t off    = am2_shift_imm(addr_read_reg(rm), stype, shimm);
    uint32_t delta  = U ? off : (uint32_t)(-((int32_t)off));
    uint32_t addr   = P ? (base + delta) : base;
    uint32_t new_rn = base + delta;

    uint8_t  val = (uint8_t)(cpu.r[rd] & 0xFFu);
    mem_write8(addr, val);
    if (W || !P) cpu.r[rn] = new_rn;

    if (debug_flags & DBG_MEM_WRITE) {
        static const char* S[4] = {"LSL","LSR","ASR","ROR"};
        log_printf("[STRB reg/shift] [0x%08X] <= r%u(0x%02X)  (r%d %c= %s #%u%s)\n",
                   addr, rd, val, rn, U?'+':'-', S[stype], shimm, (W||!P)?" wb":"");
    }
}

// ---- SWP / SWPB (uniprocessor semantics) ----
// SWP  Rd, Rm, [Rn]    ; atomically: tmp=[Rn]; [Rn]=Rm; Rd=tmp
void handle_swp(uint32_t instr) {
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;

    uint32_t addr = addr_read_reg(Rn);
    uint32_t old  = mem_read32(addr);
    mem_write32(addr, cpu.r[Rm]);
    if (Rd != 15u) cpu.r[Rd] = old;  // Rd==PC: ignore (keep VM robust)
    if (debug_flags & DBG_MEM_WRITE)
        log_printf("[SWP] r%u<=0x%08X; [0x%08X]<=r%u(0x%08X)\n",
                   Rd, old, addr, Rm, cpu.r[Rm]);
}

// SWPB byte variant
void handle_swpb(uint32_t instr) {
    uint32_t Rn = (instr >> 16) & 0xFu;
    uint32_t Rd = (instr >> 12) & 0xFu;
    uint32_t Rm =  instr        & 0xFu;

    uint32_t addr = addr_read_reg(Rn);
    uint8_t  old  = mem_read8(addr);
    mem_write8(addr, (uint8_t)(cpu.r[Rm] & 0xFFu));
    if (Rd != 15u) cpu.r[Rd] = (uint32_t)old;
    if (debug_flags & DBG_MEM_WRITE)
        log_printf("[SWPB] r%u<=0x%02X; [0x%08X]<=r%u(0x%02X)\n",
                   Rd, old, addr, Rm, (unsigned)(cpu.r[Rm] & 0xFFu));
}