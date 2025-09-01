import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_clz"

CHECKS = [
    # Stable .word decodes
    ("MRS @00008008", "00008008:       E10FC000        .word 0xE10FC000"),
    ("CLZ @00008010", "00008010:       E16F0F11        .word 0xE16F0F11"),
    ("CLZ @0000801C", "0000801C:       E16F0F11        .word 0xE16F0F11"),
    ("CLZ @0000802C", "0000802C:       E16F0F11        .word 0xE16F0F11"),
    ("CLZ @0000803C", "0000803C:       E16F0F11        .word 0xE16F0F11"),
    ("CLZ @0000804C", "0000804C:       E16F0F11        .word 0xE16F0F11"),
    ("CLZ @0000805C", "0000805C:       E16F0F11        .word 0xE16F0F11"),
    ("MRS @00008064", "00008064:       E10FB000        .word 0xE10FB000"),
    ("EOR @00008068", "00008068:       E02CA00B        .word 0xE02CA00B"),
    ("BKPT @00008070","00008070:       E1212374        .word 0xE1212374"),

    # Register dump (spot checks; confirms dump exists + final CLZ result)
    ("Regs r0 final", "r0  = 0x00000003"),
    ("Regs r15 tail", "r15 = 0x00008070"),
    ("CPSR line",     "CPSR = 0x00000000"),
    ("Cycle line",    "cycle=29"),
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