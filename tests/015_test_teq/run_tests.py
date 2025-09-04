import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_teq"

CHECKS = [
    # Binary load confirmation
    ("Loaded TEQ binary", "[LOAD] test_teq.bin @ 0x00008000"),

    # TEQ instruction decode sightings (3 cases)
    ("TEQ decoded #1", "[K12] TEQ match (key=0x130)"),
    ("TEQ decoded #2", "[K12] TEQ match (key=0x130)"),
    ("TEQ decoded #3", "[K12] TEQ match (key=0x130)"),

    # BKPT halts after each case
    ("Halt after case 1", "BKPT match (key=0x127)"),
    ("Halt after case 2", "BKPT match (key=0x127)"),
    ("Halt after case 3", "BKPT match (key=0x127)"),

    # Register markers for each case
    ("Case 1 marker r2=1", "r2  = 0x00000001"),
    ("Case 2 marker r2=2", "r2  = 0x00000002"),
    ("Case 3 marker r2=3", "r2  = 0x00000003"),

    # CPSR flag results
    ("Case 1 CPSR Z=1", "CPSR = 0x40000000"),
    ("Case 2 CPSR Z=0,N=0", "CPSR = 0x00000000"),
    ("Case 3 CPSR N=1", "CPSR = 0x80000000"),
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