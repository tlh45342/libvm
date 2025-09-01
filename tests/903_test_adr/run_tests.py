import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_adc"

CHECKS = [
    # decodes
    ("MOVW decoded",         "[K12] MOVW match (key=0x300)"),
    ("MOVT decoded",         "[K12] MOVT match (key=0x341)"),
    ("MOV imm decoded",      "[K12] MOV (imm) match (key=0x3A0)"),
    ("SUB (reg) decoded",    "[K12] SUB match (key=0x050)"),
    ("SUBS (imm) decoded",   "[K12] SUB match (key=0x250)"),
    ("ADC (reg) decoded",    "[K12] ADC match (key=0x0B0)"),
    ("ADC (sh reg) decoded", "[K12] ADC match (key=0x0B8)"),
    ("MRS decoded",          "[K12] MRS match (key=0x100)"),
    ("MVN decoded",          "[K12] MVN match (key=0x1E0)"),

    # memory effects
    ("STR +0",               "mem[0x00100000] <= r5 (0x00000004)"),
    ("STR +4",               "mem[0x00100004] <= r7 (0x00000000)"),
    ("STR +8",               "mem[0x00100008] <= r5 (0x00000000)"),
    ("STR +12",              "mem[0x0010000C] <= r7 (0x60000000)"),
    ("STR +16",              "mem[0x00100010] <= r5 (0x80000000)"),
    ("STR +20",              "mem[0x00100014] <= r7 (0x90000000)"),
    ("STR +24",              "mem[0x00100018] <= r5 (0x00000000)"),
    ("STR +28",              "mem[0x0010001C] <= r7 (0x70000000)"),
    ("STR +32",              "mem[0x00100020] <= r5 (0x00000008)"),
    ("STR +36",              "mem[0x00100024] <= r7 (0x00000000)"),

    # halt + final state
    ("DEADBEEF trap",        "DEADBEEF"),
    ("Final r3==5",          "r3  = 0x00000005"),
    ("Final r4==1",          "r4  = 0x00000001"),
    ("Final r5==8",          "r5  = 0x00000008"),
    ("Final r6 base",        "r6  = 0x00100000"),
    ("Final r7==0",          "r7  = 0x00000000"),
    ("Final PC",             "r15 = 0x000080BC"),
    ("Final CPSR",           "CPSR = 0x00000000"),
    ("Cycle count",          "cycle=48"),
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