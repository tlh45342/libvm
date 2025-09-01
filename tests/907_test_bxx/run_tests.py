import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_bxx"

CHECKS = [
    # breadcrumbs for base setup
    ("DIS 8000 MOVW r6", "00008000:       E3006000"),
    ("DIS 8004 MOVT r6", "00008004:       E3406100"),

    # taken branches -> tags 1..6
    ("EQ taken -> 1",    "mem[0x01000000] <= r1 (0x00000001)"),
    ("NE taken -> 2",    "mem[0x01000004] <= r1 (0x00000002)"),
    ("HS taken -> 3",    "mem[0x01000008] <= r1 (0x00000003)"),
    ("LO taken -> 4",    "mem[0x0100000C] <= r1 (0x00000004)"),
    ("MI taken -> 5",    "mem[0x01000010] <= r1 (0x00000005)"),
    ("PL taken -> 6",    "mem[0x01000014] <= r1 (0x00000006)"),

    # V-set case: BVS should NOT be taken -> 0xEE
    ("BVS not taken",    "B cond fail (0x6)"),
    ("VS not taken ->EE","mem[0x01000018] <= r1 (0x000000EE)"),

    # V-clear case: BVC taken -> 8
    ("VC taken -> 8",    "mem[0x0100001C] <= r1 (0x00000008)"),

    # signed compares
    ("GE taken -> 9",    "mem[0x01000020] <= r1 (0x00000009)"),
    ("LT taken -> 10",   "mem[0x01000024] <= r1 (0x0000000A)"),
    ("GT taken -> 11",   "mem[0x01000028] <= r1 (0x0000000B)"),
    ("LE taken -> 12",   "mem[0x0100002C] <= r1 (0x0000000C)"),

    # unsigned compares
    ("HI taken -> 13",   "mem[0x01000030] <= r1 (0x0000000D)"),
    ("LS taken -> 14",   "mem[0x01000034] <= r1 (0x0000000E)"),

    # halt
    ("HALT line",        "DEADBEEF        .word 0xDEADBEEF"),
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