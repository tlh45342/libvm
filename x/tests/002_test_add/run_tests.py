import subprocess
import os
import sys

VM = "arm-vm.exe"

TEST_NAME = "test_add"

CHECKS = [
    # disassembly breadcrumbs (match the exact new format)
    ("DISASM 8000 MOV",  "00008000:       E3A00000"),
    ("DISASM 8004 ADD",  "00008004:       E2801005"),
    ("DISASM 8008 ADD",  "00008008:       E2812007"),
    ("DISASM 800C MOV",  "0000800C:       E3A0300A"),
    ("DISASM 8010 ADD",  "00008010:       E0824003"),
    ("DISASM 8014 MVN",  "00008014:       E3E05000"),
    ("DISASM 8018 ADD",  "00008018:       E2856001"),
    ("DISASM 801C HALT", "0000801C:       DEADBEEF"),

    # halt + final state
    ("HALT",             "DEADBEEF"),
    ("Reg r4=22",        "r4  = 0x00000016"),
    ("Reg r6=0",         "r6  = 0x00000000"),
    ("PC final",         "r15 = 0x0000801C"),
    ("CPSR zero",        "CPSR = 0x00000000"),
    ("Cycle count",      "cycle=8"),
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