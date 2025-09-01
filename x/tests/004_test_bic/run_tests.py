import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_bic"

CHECKS = [
    # Early decodes
    ("MOV imm",              "[K12] MOV (imm) match (key=0x3AF)"),  # E3A000F0
    ("TEQ (imm)",            "[K12] TEQ match (key=0x320)"),        # E328F000
    ("BIC (imm) #1",         "[K12] BIC (imm) match (key=0x3DF)"),  # E3D010F0
    ("MRS #1",               "[K12] MRS match (key=0x100)"),        # E10FA000
    ("AND mask r10",         "[K12] AND (imm) match (key=0x200)"),  # E20AA20F (flags nibble)

    # More BIC/TEQ/MRS coverage
    ("BIC (imm) #2",         "[K12] BIC (imm) match (key=0x3D0)"),  # E3D02102
    ("MRS #2",               "PC=0x00008018"),
    ("AND mask r11",         "E20BB20F"),
    ("MOV imm r3",           "E3A03001"),
    ("ORR imm r3",           "[K12] ORR (imm) match (key=0x380)"),  # E3833102

    # Register forms
    ("BIC (reg) #1",         "[K12] BIC match (key=0x1D8)"),        # E1D35F84
    ("MRS #3",               "PC=0x00008030"),
    ("AND mask r12",         "E20CC20F"),
    ("MRS #4",               "PC=0x00008038"),
    ("ORR imm r9",           "E3899202"),
    ("TEQ (reg)",            "[K12] TEQ match (key=0x120)"),        # E128F009

    # Clear & BIC again
    ("MOV imm r6=0",         "E3A06000"),
    ("BIC (reg) #2",         "[K12] BIC match (key=0x1D0)"),        # E1D07006
    ("MRS #5",               "PC=0x0000804C"),
    ("AND mask r13",         "E20DD20F"),

    # Final MOV (reg) sanity + halt
    ("MOV (reg) final",      "[K12] MOV match (key=0x1A0)"),        # E1A00000
    ("DEADBEEF trap",        "[K12] DEADBEEF match"),
    ("DEADBEEF PC",          "PC=0x00008058"),
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