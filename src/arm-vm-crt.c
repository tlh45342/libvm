// src/arm-vm-crt.c — hardcoded CRT runner (run.bin + floppy.img, 512 MiB RAM)
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#  include <windows.h>
static void sleep_ms(unsigned ms){ Sleep(ms); }
static void pump_win_msgs(void){
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#else
#  include <time.h>
static void sleep_ms(unsigned ms){
    struct timespec ts = { ms/1000, (long)(ms%1000)*1000000L };
    nanosleep(&ts, NULL);
}
static void pump_win_msgs(void){ /* no-op on non-Windows */ }
#endif

#include "vm.h"
#include "log.h"
#include "dev_disk.h"
#include "dev_crt.h"

#define LOAD_ADDR 0x00008000u
#define START_PC  0x00008000u
#define CRT_MMIO  0x09000000u
#define CRT_COLS  80
#define CRT_ROWS  25

int main(void) {
    const char *bin  = "run.bin";
    const char *disk = "floppy.img";

    VM *vm = vm_create();
    if (!vm) { fprintf(stderr, "vm_create failed\n"); return 1; }
    vm_reset(vm);

    if (!vm_add_ram(vm, 512u * 1024u * 1024u)) {
        fprintf(stderr, "vm_add_ram(512MiB) failed\n");
        vm_destroy(vm); return 1;
    }

    // Optional disk (ignore failure if missing)
    dev_disk0_attach(vm, disk);

    // Init CRT at 0x09000000 and explicitly enable/show it
    if (!dev_crt_init(CRT_COLS, CRT_ROWS, CRT_MMIO)) {
        fprintf(stderr, "dev_crt_init failed\n");
    } else {
        dev_crt_set_enabled(true);   // <-- important: actually show the window
        // give Windows a moment to create/show
        for (int i = 0; i < 10; ++i) { pump_win_msgs(); sleep_ms(16); }
    }

    if (!vm_load_binary(vm, bin, LOAD_ADDR)) {
        fprintf(stderr, "load failed: %s\n", bin);
        vm_destroy(vm); return 1;
    }
    vm_set_reg(vm, 15, START_PC);
    log_printf("Set r15 = 0x%08X\n", START_PC);

    vm_run(vm, 0);
    log_printf("[INFO] HALTed. Keeping CRT window open.\n");

    // Keep the window responsive indefinitely
    for (;;) {
        pump_win_msgs();
        sleep_ms(16);
    }
}
