import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_mvn"

CHECKS = [
    # ---- MVN variants caught by the decoder ----
    ("MVN imm #1 (.word)",       "00008000:       E3E00000        .word 0xE3E00000"),
    ("MVN reg (.word)",          "00008010:       E1E03001        .word 0xE1E03001"),
    ("MVN imm #2 (.word)",       "00008020:       E3F04000        .word 0xE3F04000"),
    ("MVN RRX #1 (.word)",       "00008030:       E1F050A0        .word 0xE1F050A0"),
    ("MVN RRX #2 (.word)",       "00008044:       E1F06060        .word 0xE1F06060"),
    ("MVN LSL(reg) #1 (.word)",  "00008058:       E1F07110        .word 0xE1F07110"),
    ("MVN LSL(reg) #2 (.word)",  "00008070:       E1F08110        .word 0xE1F08110"),

    # ---- Fences & flag-probe TEQs to anchor the flow ----
    ("Fence #1",                 "00008008:       E320F000        .word 0xE320F000"),
    ("Fence #2",                 "00008018:       E320F000        .word 0xE320F000"),
    ("TEQ snap #1",              "0000801C:       E328F202        .word 0xE328F202"),
    ("Fence #3",                 "00008028:       E320F000        .word 0xE320F000"),
    ("Fence #4",                 "00008038:       E320F000        .word 0xE320F000"),
    ("TEQ snap #2",              "0000803C:       E328F202        .word 0xE328F202"),
    ("Fence #5",                 "0000804C:       E320F000        .word 0xE320F000"),
    ("Fence #6",                 "00008060:       E320F000        .word 0xE320F000"),
    ("TEQ snap #3",              "00008064:       E328F202        .word 0xE328F202"),
    ("Fence #7",                 "00008078:       E320F000        .word 0xE320F000"),

    # ---- Halt ----
    ("BKPT decoded",             "0000807C:       E1200070        .word 0xE1200070"),

    # ---- Final state snapshot assertions ----
    ("CPSR final",               "CPSR = 0x80000000"),
    ("r0 final",                 "r0  = 0x00000055"),
    ("r1 final",                 "r1  = 0x00000000"),
    ("r2 final",                 "r2  = 0x00000097"),
    ("r3 final",                 "r3  = 0xFFFFFF55"),
    ("r4 final",                 "r4  = 0xFFFFFFFF"),
    ("r5 final",                 "r5  = 0xFFFFFFFF"),
    ("r6 final",                 "r6  = 0xFFFFFFFF"),
    ("r7 final",                 "r7  = 0xFFFFFFFD"),
    ("r8 final",                 "r8  = 0xFFFFFFAA"),
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