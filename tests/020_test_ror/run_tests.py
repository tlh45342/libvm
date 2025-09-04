import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_ror"

CHECKS = [
    ("Loaded image",           "[LOAD] test_ror.bin @ 0x00008000"),
    ("PC start",               "r15 <= 0x00008000"),
    ("Base set (MOVT r6)",     "00008004:       E3406010"),
    ("ROR/rrx MOVS",           "00008010:       E1B010E0"),
    ("MRS #1",                 "00008014:       E10F7000"),
    ("STR r1,[r6,#0]",         "00008018:       E5861000"),
    ("ROR reg MOVS",           "00008020:       E1B02460"),
    ("MRS #2",                 "00008024:       E10F7000"),
    ("BKPT",                   "00008030:       E1212374"),
    ("Final PC",               "r15 = 0x00008030"),
    ("Final CPSR",             "CPSR ="),
    ("Cycle count=13",         "cycle=13"),
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