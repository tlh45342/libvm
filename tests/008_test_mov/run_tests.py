import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_mov"

CHECKS = [
    # Fences (NOPs) after each sub-case
    ("Fence #1 after MOV imm",           "00008008:       E320F000        .word 0xE320F000"),
    ("Fence #2 after MOV r1,r0",         "00008014:       E320F000        .word 0xE320F000"),
    ("Fence #3 after MOV r3,r0,lsl #1",  "00008020:       E320F000        .word 0xE320F000"),
    ("Fence #4 after MOVS r4,#1",        "00008030:       E320F000        .word 0xE320F000"),
    ("Fence #5 mid RRX/MOVS block",      "00008040:       E320F000        .word 0xE320F000"),
    ("Fence #6 after RRX test",          "00008054:       E320F000        .word 0xE320F000"),
    ("Fence #7 after next shift test",   "00008064:       E320F000        .word 0xE320F000"),
    ("Fence #8 before LSL (reg) test",   "00008078:       E320F000        .word 0xE320F000"),
    ("Fence #9 before final LSL (reg)",  "0000808C:       E320F000        .word 0xE320F000"),

    # Halt
    ("BKPT decoded",                     "000080A8:       E1200070        .word 0xE1200070"),

    # Final state snapshot assertions
    ("CPSR final",                       "CPSR = 0x00000000"),
    ("r0 final",                         "r0  = 0x00000055"),
    ("r1 final",                         "r1  = 0x00000000"),
    ("r2 final",                         "r2  = 0x0000008A"),
    ("r3 final",                         "r3  = 0x00000084"),
    ("r4 final",                         "r4  = 0x00000001"),
    ("r8 final",                         "r8  = 0x00000002"),
    ("r9 final",                         "r9  = 0x00000055"),
]

def run_test():
    print(f"Running {TEST_NAME}...")

    script_path = f"{TEST_NAME}.script"
    bin_path = f"{TEST_NAME}.bin"
    log_path = f"{TEST_NAME}.log"

    if not os.path.exists(script_path):
        print(f"❌ Missing script: {script_path}")
        return False

    if not os.path.exists(bin_path):
        print(f"❌ Missing binary: {bin_path}")
        return False

    try:
        subprocess.run(
            [VM],
            stdin=open(script_path, "r"),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
    except FileNotFoundError:
        print(f"❌ Error: '{VM}' not found in PATH.")
        return False

    if not os.path.exists(log_path):
        print(f"❌ Missing log file: {log_path}")
        return False

    with open(log_path, "r") as f:
        log = f.read()

    passed = True
    for label, expected in CHECKS:
        if expected not in log:
            print(f"  ❌ Check failed: {label}")
            print(f"     Missing: {expected}")
            passed = False
        else:
            print(f"  ✅ {label}")

    print(f"{TEST_NAME}: {'✅ passed' if passed else '❌ failed'}\n")
    return passed

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)