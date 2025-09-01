import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_adc"

CHECKS = [
    # setup / config
    ("Debug enabled",            "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",             "[LOAD] test_adc.bin @ 0x00008000"),
    ("PC start",                 "r15 <= 0x00008000"),

    # early decodes (front of program)
    ("MOVW decoded",             "[K12] MOVW match"),
    ("MOVT decoded",             "[K12] MOVT match"),
    ("MOV imm decoded",          "[K12] MOV (imm) match"),

    # arithmetic paths we care about in this test
    ("SUB (reg) decoded",        "[K12] SUB match (key=0x050)"),   # E050C000
    ("SUBS (imm) decoded",       "[K12] SUB match (key=0x250)"),   # E250C001
    ("ADC (reg) decoded #1",     "[K12] ADC match (key=0x0B0)"),   # E0B35004
    ("ADC (reg) decoded #2",     "[K12] ADC match (key=0x0B8)"),   # E0B35084 (shifted)

    # PSR snapshot reads
    ("MRS decoded #1",           "[K12] MRS match (key=0x100)"),
    ("MRS decoded #2",           "Instr=0xE10F7000"),

    # observable memory effects (golden values)
    ("STR r5 @ +0",              "mem[0x00100000] <= r5 (0x00000004)"),
    ("STR r7 @ +4",              "mem[0x00100004] <= r7 (0x00000000)"),
    ("STR r5 @ +8",              "mem[0x00100008] <= r5 (0x00000000)"),
    ("STR r7 @ +12",             "mem[0x0010000C] <= r7 (0x60000000)"),
    ("STR r5 @ +16",             "mem[0x00100010] <= r5 (0x80000000)"),
    ("STR r7 @ +20",             "mem[0x00100014] <= r7 (0x90000000)"),
    ("STR r5 @ +24",             "mem[0x00100018] <= r5 (0x00000000)"),
    ("STR r7 @ +28",             "mem[0x0010001C] <= r7 (0x70000000)"),
    ("STR r5 @ +32",             "mem[0x00100020] <= r5 (0x00000008)"),
    ("STR r7 @ +36",             "mem[0x00100024] <= r7 (0x00000000)"),

    # graceful stop
    ("DEADBEEF trap",            "[K12] DEADBEEF match"),

    # final machine state (spot-check key regs + cpsr + cycle)
    ("Final r3==5",              "r3  = 0x00000005"),
    ("Final r4==1",              "r4  = 0x00000001"),
    ("Final r5==8",              "r5  = 0x00000008"),
    ("Final r6 base",            "r6  = 0x00100000"),
    ("Final r7==0",              "r7  = 0x00000000"),
    ("Final PC",                 "r15 = 0x000080BC"),
    ("Final CPSR",               "CPSR = 0x00000000"),
    ("Cycle count",              "cycle=48"),
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