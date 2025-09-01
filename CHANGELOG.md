# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]
- Planned: ext2/ext3 filesystem utilities
- Planned: improved automation testing framework
- Planned: scheduler & interrupt support

  ## 2025-08-24

  - Added **ADC/ADCS** and **SBC/SBCS** instruction support:
    - Updated `execute()` decode masks to accept both `S=0` and `S=1` forms.
    - Unified handling via `exec_arith()` core.
  - Added **TST (register/shifted register)** decoding and handler.
  - Created new test cases in `/tests`:
    - `36_test_adcs/` — verifies ADC + ADCS behavior (carry, overflow, flags).
    - `37_test_sbcs/` — verifies SBC + SBCS behavior (borrow/flags).
    - `38_test_tst/` — verifies TST reg form, including shifter carry edge cases.
  - Cleaned up duplicate helpers (`set_NZ`, `dp_decode_operand2`) and redundant prototypes.
  - Fixed CPS decode warning by bracing condition in `execute()`.
  - Verified with logs: `BICS`, `ORR`, `MRS`, `ADC`, `SBC`, and `TST` test programs run correctly.

  ## 2025-08-23

  - Added **BIC/BICS** instruction support.
  - Added `35_test_bics/` to validate immediate, shifted register, and RRX cases.
  - Implemented special case for `BICS pc, …` to restore CPSR from SPSR in non-user modes.
  - Verified flags and results match ARM ARM behavior.

---

## [0.0.56] - 2025-07-27
### Added
- Correct handling of `LDRB` post-indexed addressing.

### Fixed
- Instruction decoding improvements.

---

## [0.0.47] - 2025-07-27
### Added
- Support for `LDM` instruction variants.
- Backtrace (`bt`) command in REPL.
- Robust error handling for invalid memory access and unknown instructions.
- `CRT` display and `UART` output devices.

### Changed
- Improved stack management with bounds checking.
- Refined `execute()` dispatch logic.
- WFI (`0xE320F000`) treated as NOP with debug logging.

---

## [0.0.39] - 2025-07-22
### Added
- Handler for `LDRB rX, [rY]` (`handle_ldrb_reg`).
- Halt reporting for unknown instructions.

---

## [0.0.32] - 2025-07-20
### Added
- `do` command in REPL shell (SIMH-like) to run script files.

### Changed
- Removed `[VM halted]` message from REPL.
- Refined `execute()` STR pre-indexed decoding.

---

## [0.0.29] - 2025-07-20
### Added
- Stack safety checks and backtrace support.

---

## [0.0.21] - 2025-07-20
### Added
- Initial ARM VM emulator with instruction decoding for:
  - `MOV`, `ADD`, `SUB`, `LDR`, `STR`, `BL`
- Simulated memory space with MMIO handling.
- REPL shell with commands (`load`, `run`, `regs`, `set`, etc.).
- Debug flags (`DBG_INSTR`, `DBG_MEM`, etc.).
- Binary loader starting execution at `0x8000`.

---

## [0.0.1] - 2025-07-?? (inception)
### Added
- First experimental build of the ARM VM emulator.