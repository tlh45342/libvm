import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_orr"

CHECKS = [
    # setup / config
    ("Debug enabled",        "[DEBUG] debug_flags set to 0x000003FF"),
    ("Loaded image",         "[LOAD] test_orr.bin @ 0x00008000"),
    ("PC start",             "r15 <= 0x00008000"),

    # decode / flow (match your trace exactly)
    ("8000 mov",             "00008000:       E3A000F0        mov r0, #0xF0"),
    ("8004 orr imm",         "00008004:       E380100F        orr r1, r0, #0xF"),
    ("8008 mov",             "00008008:       E3A0200F        mov r2, #0xF"),
    ("800C orr reg",         "0000800C:       E1813002        .word 0xE1813002"),  # ORR (reg)
    ("8010 mov",             "00008010:       E3A04000        mov r4, #0x0"),
    ("8014 orr imm",         "00008014:       E3844001        orr r4, r4, #0x1"),
    ("8018 mov",             "00008018:       E3A05001        mov r5, #0x1"),
    ("801C orr shift(S)",    "0000801C:       E1946085        .word 0xE1946085"),  # ORRS r6, r4, r5, lsl #1
    ("8020 mrs cpsr",        "00008020:       E10FA000        .word 0xE10FA000"),
    ("8024 orr cond",        "00008024:       038770FF        orr r7, r7, #0xFF"), # cond (EQ) not taken
    ("8028 mov cond",        "00008028:       03A07001        mov r7, #0x1"),      # cond (EQ) not taken

    # graceful stop
    ("BKPT trap",            "BKPT"),  # or match the line above; keep whichever your harness expects
]

def run_test():
    print(f"Running {TEST_NAME}.script...")

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