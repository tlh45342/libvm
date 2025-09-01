import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_cmn"

CHECKS = [
    # 1) CMN(reg) baseline: 0 + 0 → Z=1 only (0x4000_0000)
    ("CMN #1 decode",        "[K12] CMN match (key=0x170)"),
    ("Flags #1 stored",      "mem[0x00100000] <= r7 (0x40000000)"),

    # 2) MVN then CMN(reg): 0xFFFF_FFFF + 1 → Z=1, C=1 (0x6000_0000)
    ("MVN prep decode",      "[K12] MVN match (key=0x1E0)"),
    ("CMN #2 decode",        "[K12] CMN match (key=0x170)"),
    ("Flags #2 stored",      "mem[0x00100004] <= r7 (0x60000000)"),

    # 3) CMN(reg) overflow case: produces N=1, V=1 (0x9000_0000)
    ("CMN overflow decode",  "[K12] CMN match (key=0x170)"),
    ("Flags overflow stored","mem[0x00100008] <= r7 (0x90000000)"),

    # 4) CMN(imm/shift RRX path): no flags set (0x0000_0000)
    ("CMN RRX decode",       "[K12] CMN match (key=0x178)"),
    ("Flags RRX stored",     "mem[0x00100010] <= r7 (0x00000000)"),

    # 5) CMN variant setting N only (0x8000_0000)
    ("CMN N-only decode",    "[K12] CMN match (key=0x176)"),
    ("Flags N-only stored",  "mem[0x00100014] <= r7 (0x80000000)"),

    # Trap: BKPT decode + final PC
    ("BKPT decode",          "[K12] BKPT match (key=0x127)"),
    ("BKPT PC",              "PC=0x000080AC"),
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