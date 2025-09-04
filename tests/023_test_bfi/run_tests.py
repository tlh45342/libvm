import subprocess
import os
import sys

VM = "arm-vm.exe"
TEST_NAME = "test_bfi"

CHECKS = [
    # setup
    ("Loaded image",          "[LOAD] test_bfi.bin @ 0x00008000"),
    ("PC start",              "r15 <= 0x00008000"),
    ("Base set (MOVT r6)",    "00008004:       E3406010"),

    # BFI sequences + stores (just prove they executed)
    ("BFI #1 opcode",         "00008018:       E7C30011"),
    ("STR r0 -> [r6,#0]",     "0000801C:       E5860000"),

    ("BFI #2 opcode",         "0000802C:       E7CF2413"),
    ("STR r2 -> [r6,#4]",     "00008030:       E5862004"),

    ("BFI #3 opcode",         "00008044:       E7D74615"),
    ("STR r4 -> [r6,#8]",     "00008048:       E5864008"),

    ("BFI #4 opcode",         "0000805C:       E7DF7018"),
    ("STR r7 -> [r6,#12]",    "00008060:       E586700C"),

    ("BFI #5 opcode",         "00008070:       E7DF9E1A"),
    ("STR r9 -> [r6,#16]",    "00008074:       E5869010"),

    # graceful stop
    ("BKPT",                  "00008078:       E1212374"),

    # final machine state (stable anchors)
    ("Final PC",              "r15 = 0x00008078"),
    ("Final CPSR",            "CPSR = 0x00000000"),
    ("Cycle count",           "cycle=31"),
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