#include "cond.h"
#include "cpu.h"

extern CPU cpu;

bool evaluate_condition(uint8_t cond) {
    bool N = (cpu.cpsr >> 31) & 1;
    bool Z = (cpu.cpsr >> 30) & 1;
    bool C = (cpu.cpsr >> 29) & 1;
    bool V = (cpu.cpsr >> 28) & 1;

    switch (cond) {
        case 0x0: return Z;               // EQ
        case 0x1: return !Z;              // NE
        case 0x2: return C;               // CS
        case 0x3: return !C;              // CC
        case 0x4: return N;               // MI
        case 0x5: return !N;              // PL
        case 0x6: return V;               // VS
        case 0x7: return !V;              // VC
        case 0x8: return C && !Z;         // HI
        case 0x9: return !C || Z;         // LS
        case 0xA: return N == V;          // GE
        case 0xB: return N != V;          // LT
        case 0xC: return !Z && (N == V);  // GT
        case 0xD: return Z || (N != V);   // LE
        case 0xE: return true;            // AL
        default:  return false;           // NV / undefined
    }
}