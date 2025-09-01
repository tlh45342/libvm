import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_cmp"

CHECKS = [
    ("Loaded",               "[LOAD] test_cmp.bin @ 0x00008000"),
    ("PC start",             "r15 <= 0x00008000"),

    # Case 1: CMP r1, r2 ; then snapshot CPSR
    ("CMP r1,r2 opcode",     "E1510002        .word 0xE1510002"),
    ("MRS after case1",      "E10F0000        .word 0xE10F0000"),

    # Case 2: CMP r0, #2 ; snapshot
    ("MRS after case2",      "E10F1000        .word 0xE10F1000"),

    # Case 3: CMP r0, #1 ; snapshot
    ("MRS after case3",      "E10F2000        .word 0xE10F2000"),

    # Case 4: CMP r2, r3, LSL #4 ; snapshot
    ("CMP r2,r3,LSL#4 opcode","E1520203        .word 0xE1520203"),
    ("MRS after case4",      "E10F4000        .word 0xE10F4000"),

    # Case 5: (prep with MVN), CMP r4,#0 ; snapshot
    ("MVN prep opcode",      "E3E04000        .word 0xE3E04000"),
    ("MRS after case5",      "E10F5000        .word 0xE10F5000"),

    # Halt
    ("BKPT decoded",         "E1212374        .word 0xE1212374"),
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