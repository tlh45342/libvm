import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_lsl"

CHECKS = [
    ("Loaded image",           "[LOAD] test_lsl.bin @ 0x00008000"),
    ("PC start",               "r15 <="),
    ("Base set (MOVT r6)",     "MOVT match"),
    ("LSLS #1",                "LSLS"),
    ("CPSR #1 stored",         "STR r3, [r6,#0]"),
    ("LSLS #0 (C unchanged)",  "LSLS"),
    ("CPSR #2 stored",         "STR r3, [r6,#4]"),
    ("LSLS #31",               "LSLS"),
    ("CPSR #3 stored",         "STR r3, [r6,#8]"),
    ("LSLS #32 (imm)",         "LSLS"),
    ("CPSR #4 stored",         "STR r3, [r6,#12]"),
    ("LSLS reg #33",           "LSLS"),
    ("CPSR #5 stored",         "STR r3, [r6,#16]"),
    ("BKPT",                   "[BKPT]"),
    ("Final PC",               "Final PC"),
    ("Final CPSR",             "Final CPSR"),
    ("Cycle count",            "Cycle count"),
]

def run_test():
    print(f"Running {TEST_NAME}.script...")

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