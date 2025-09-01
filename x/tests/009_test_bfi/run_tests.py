import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_bfi"

CHECKS = [
    ("Loaded",              "[LOAD] test_bfi.bin @ 0x00008000"),
    ("BFI key 7C1 first",   "[K12] BFI match (key=0x7C1)"),
    ("BFI result #0",       "mem[0x00100000] <= r0 (0x1234567A)"),
    ("BFI key 7C1 second",  "[K12] BFI match (key=0x7C1)"),
    ("BFI result #1",       "mem[0x00100004] <= r2 (0xDEAD12EF)"),
    ("BFI key 7D1 first",   "[K12] BFI match (key=0x7D1)"),
    ("BFI result #2",       "mem[0x00100008] <= r4 (0xFFBDF000)"),
    ("BFI key 7D1 second",  "[K12] BFI match (key=0x7D1)"),
    ("BFI result #3",       "mem[0x0010000C] <= r7 (0x0F0F0F0F)"),
    ("BFI key 7D1 third",   "[K12] BFI match (key=0x7D1)"),
    ("BFI result #4",       "mem[0x00100010] <= r9 (0xC1234567)"),
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