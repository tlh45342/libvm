import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_asr"

CHECKS = [
    # setup
    ("Loaded image",          "[LOAD] test_asr.bin @ 0x00008000"),
    ("PC start",              "r15 <="),

    # r3 = 0x00008080; r4 = 8 -> ASRS reg (mid shift)
    ("@809C movt r3,#0",      "0000809C:       E3403000"),
    ("@80A0 mov r4,#8",       "000080A0:       E3A04008"),
    ("@80A4 ASRS reg",        "000080A4:       E1B05453"),
    ("@80A8 MRS",             "000080A8:       E10F7000"),

    # r3 = 0xF0000000; r4 = 40 (>=32) -> ASRS reg >=32
    ("@80B4 movw r3,#0",      "000080B4:       E3003000"),
    ("@80B8 movt r3,#0xF000", "000080B8:       E34F3000"),
    ("@80BC mov r4,#40",      "000080BC:       E3A04028"),
    ("@80C0 ASRS reg >=32",   "000080C0:       E1B05453"),
    ("@80C4 MRS",             "000080C4:       E10F7000"),

    # halt
    ("DEADBEEF trap",         "000080D0:       DEADBEEF"),
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