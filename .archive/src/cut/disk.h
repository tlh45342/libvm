// src/disk.h
#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdbool.h>
#include "arm-vm.h"   // brings in DISK_BASE, DISK_MMIO_SIZE, DISK_REG_* macros

// Backing store for the disk image (defined in disk.c)
extern uint8_t disk_data[];

// Public API
bool     load_disk_image(const char *path);
void     handle_disk_write(uint32_t addr, uint32_t value);
uint32_t handle_disk_read(uint32_t addr);

#endif // DISK_H
