#pragma once
#include <stdint.h>

void     cpu_bt_push(uint32_t lr);
void     cpu_bt_pop(void);
int      cpu_bt_depth(void);
uint32_t cpu_bt_frame(int i);
void     cpu_dump_backtrace(void);