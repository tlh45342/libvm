import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_bfc"

CHECKS = [
    ("BFC all bits",        "mem[0x00100000] <= r0 (0x00000000)"),
    ("BFC byte 8..15",      "mem[0x00100004] <= r1 (0x123400CD)"),
    ("BFC nibble 4..7",     "mem[0x00100008] <= r2 (0x87654301)"),
    ("BFC bits 20..27",     "mem[0x0010000C] <= r3 (0xF000F0F0)"),
    ("BKPT",                "[K12] BKPT match"),
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