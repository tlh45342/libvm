// mem.h
#ifndef MEM_H
#define MEM_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void   mem_init(void);                 // <-- add this
void   mem_bind(uint8_t *base, size_t size);
void   mem_unbind(void);
bool   mem_is_bound(void);
size_t mem_size(void);

uint8_t  mem_read8 (uint32_t addr);
uint32_t mem_read32(uint32_t addr);
void     mem_write8 (uint32_t addr, uint8_t  v);
void     mem_write32(uint32_t addr, uint32_t v);

bool     mem_copy_in (uint32_t dst_addr, const void *src, size_t len);
bool     mem_copy_out(void *dst, uint32_t src_addr, size_t len);

#endif
