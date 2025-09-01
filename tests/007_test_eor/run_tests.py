import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_eor"

CHECKS = [
    # EOR (register) — .word form
    ("EOR reg (.word)",      "00008018:       E0210002        .word 0xE0210002"),

    # EORS with shifted register — .word form (captures the flags case)
    ("EORS shift (.word)",   "00008028:       E03350A4        .word 0xE03350A4"),
    ("MRS snap #1",          "0000802C:       E10F7000        .word 0xE10F7000"),

    # Second EORS with shift — .word form + second CPSR snapshot
    ("EORS shift2 (.word)",  "00008054:       E033A0C4        .word 0xE033A0C4"),
    ("MRS snap #2",          "00008058:       E10FB000        .word 0xE10FB000"),

    # Halt
    ("BKPT decoded",         "00008064:       E1212374        .word 0xE1212374"),

    # (Optional stability check on final flags)
    ("CPSR final",           "CPSR = 0xA0000000"),
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