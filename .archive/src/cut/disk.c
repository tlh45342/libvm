// src/disk.c
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "arm-vm.h"
#include "disk.h"

// Public backing store for the disk image (declared in arm-vm.h)
uint8_t disk_data[DISK_NUM_SECTORS * DISK_SECTOR_SIZE];

// ---- Private device state (file-scope) ----
static uint32_t disk_status = 0;        // 0=idle,1=busy,2=error,3=done
static uint32_t disk_lba = 0;           // sector index
static uint32_t disk_buffer_addr = 0;   // guest phys addr (DMA target)

// ---- Image loader ----
bool load_disk_image(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open disk image: %s\n", path);
        return false;
    }

    const size_t expected = (size_t)DISK_NUM_SECTORS * (size_t)DISK_SECTOR_SIZE;
    const size_t nread = fread(disk_data, 1, expected, fp);
    fclose(fp);

    if (nread < expected) {
        fprintf(stderr,
                "[WARNING] Disk image is smaller than expected (%zu/%zu bytes)\n",
                nread, expected);
    } else if (trace_all) {
        printf("[INFO] Disk image loaded: %s (%zu bytes)\n", path, nread);
    }
    return true;
}

// ---- MMIO write handler ----
void handle_disk_write(uint32_t addr, uint32_t value) {
    const uint32_t off = addr - DISK_BASE;

    switch (off) {
    case DISK_REG_CMD: {
        // value: 1 = READ sector -> DMA to ram[disk_buffer_addr]
        if (value == 1) {
            if (disk_status == 0) { // idle
                if (disk_lba < DISK_NUM_SECTORS) {
                    const uint32_t mem_addr = disk_buffer_addr;
                    const size_t disk_off = (size_t)disk_lba * DISK_SECTOR_SIZE;

                    disk_status = 1; // busy
                    memcpy(ram + mem_addr,   // <-- now using ram directly
                           disk_data + disk_off,
                           DISK_SECTOR_SIZE);
                    disk_status = 3; // done
                } else {
                    disk_status = 2; // error (LBA out of range)
                }
            }
        } else if (value == 0) {
            // reset / acknowledge
            disk_status = 0;
        }
        break;
    }

    case DISK_REG_STATUS:
        // allow guest to clear/force status
        disk_status = value;
        break;

    case DISK_REG_LBA:
        disk_lba = value;
        break;

    case DISK_REG_BUF_ADDR:
        disk_buffer_addr = value;
        break;

    default:
        // unknown register — ignore
        break;
    }
}

static inline int in_range(uint32_t addr) {
    return (addr >= DISK_BASE) && (addr < DISK_BASE + DISK_SIZE);
}

uint32_t handle_disk_read(uint32_t addr) {
    if (!in_range(addr)) return 0;

    // Provide a 32-bit little-endian value (safe for both read8 and read32 callers)
    uint32_t off = addr - DISK_BASE;
    uint32_t v = 0;

    // Avoid overruns near the end; read what we can
    if (off + 4 <= DISK_SIZE) {
        memcpy(&v, &disk_data[off], 4);
    } else {
        // Tail-safe assemble
        for (uint32_t i = 0; i < 4 && off + i < DISK_SIZE; ++i) {
            v |= (uint32_t)disk_data[off + i] << (8u * i);
        }
    }
    return v;
}

