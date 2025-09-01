import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_blbx"

CHECKS = [
    # literal/SP load
    ("LDR literal",   "[LDR literal] r13 = mem[0x00008020]"),
    ("r13 value",     "r13 = 0x00808024"),

    # call site
    ("MOV r0 imm",    "[MOV imm] r0 = 0x00000042"),
    # New BL format: "[BL] LR=0x.... -> PC=0x...."
    ("BL taken (fmt)", "[BL] LR="),
    ("BL target hop",  "-> PC="),

    # subroutine body (will appear after BL actually branches)
    ("move r1,r0",    "[MOV] r1 = r0 (0x00000042)"),
    ("r1 after mov",  "r1  = 0x00000042"),

    # return (new BX format: "[BX] r14 -> PC=0x....  (T=0)")
    ("BX taken",      "[BX] r14 -> PC="),
    ("pc after bx",   "r15 = 0x0000800C"),

    # branch to halt (multi-line block now prints "Target:" + "Branch TAKEN")
    ("B to halt target", "Target:    0x00008018"),
    ("B to halt taken",  "Branch TAKEN"),

    # halt
    ("[HALT] DEADBEEF", "[HALT] DEADBEEF"),
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