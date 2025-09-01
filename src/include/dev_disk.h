// src/include/dev_disk.h
#ifndef DEV_DISK_H
#define DEV_DISK_H
#include <stdbool.h>
#include <stdint.h>
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISK0_BASE 0x0B000000u
// regs: +0x00 CMD (1=read one sector), +0x04 LBA, +0x08 DST, +0x0C STATUS (1=busy/0=ready)

bool dev_disk0_attach(VM *vm, const char *image_path); // maps MMIO (if not already) and attaches image
bool dev_disk0_present(void);

bool     dev_disk0_present(void);
uint32_t dev_disk0_read_reg(uint32_t addr);
void     dev_disk0_write_reg(uint32_t addr, uint32_t val);

#ifdef __cplusplus
}
#endif
#endif