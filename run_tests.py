import os
import subprocess

# Top-level test directory
TEST_DIR = "tests"

# Collect all subdirectories with a run_tests.py script
tests = [
    name for name in os.listdir(TEST_DIR)
    if os.path.isdir(os.path.join(TEST_DIR, name)) and
       os.path.exists(os.path.join(TEST_DIR, name, "run_tests.py"))
]

passed = 0
failed = 0

for test in tests:
    result = subprocess.run(
        ["python", "run_tests.py"],
        cwd=os.path.join(TEST_DIR, test),
        stdout=None,  # Let it print
        stderr=None,
        text=True
    )

    if result.returncode == 0:
        passed += 1
    else:
        failed += 1

# Summary only
print(f"Summary: {passed} passed, {failed} failed")