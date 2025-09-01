import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_tst"

CHECKS = [
    ("teq (imm/reg)                 ; flag warm-up",            "00008000:       E328F000"),
    ("mov r0, #0xF0                 ; r0 = 0xF0",               "00008004:       E3A000F0"),
    ("mov r1, #0x0F                 ; r1 = 0x0F",               "00008008:       E3A0100F"),
    ("tst r0, r1                    ; Z=1",                     "0000800C:       E1100001"),
    ("mrs r10, cpsr                 ; read flags",              "00008010:       E10FA000"),
    ("and r10, r10, #0xF0000000     ; keep NZCV",               "00008014:       E20AA20F"),

    ("mov r2, #1                    ; r2 = 1",                  "00008018:       E3A02001"),
    ("tst r0, r2, lsr #1            ; Z=1, C=1 (carry from LSR)","0000801C:       E11000A2"),
    ("mrs r11, cpsr                 ; read flags",              "00008020:       E10FB000"),
    ("and r11, r11, #0xF0000000     ; keep NZCV",               "00008024:       E20BB20F"),

    ("nop                           ; spacer",                  "00008028:       E320F000"),
    ("DEADBEEF                      ; halt/sentinel",           "0000802C:       DEADBEEF"),
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