// src/arm-vm.c
#include <stdio.h>      // printf, stdin
#include <stdbool.h>    // bool, true/false
#include <stddef.h>     // NULL

#include "vm.h"         // VM*, vm_create(), vm_destroy(), vm_reset()
#include "cli.h"        // CLI, cli_init(), cli_run()
#include "dev_uart.h"        // CLI, cli_init(), cli_run()

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    // If you eventually add a logger with log_init(), include "log.h" and call it here.

    VM *vm = vm_create();          // matches src/include/vm.h: VM* vm_create(void)
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    vm_reset(vm);


	// Hard-code 512 MiB (constant already in cpu.h)
	if (!vm_add_ram(vm, 512u * 1024u * 1024u)) {
		fprintf(stderr, "vm_add_ram(512M) failed\n");
		return 1;
	}
	
	dev_uart_init(0x09000000u);           // <-- add this

	// dev_uart_write_reg(0x09000000u, (uint32_t)'!');  // TEMP: should print "!"

    CLI cli;
    cli_init(&cli, vm, stdin, true);

    cli_run(&cli);

    // If you have vm_shutdown(VM*) in vm.h, you can call it here before destroy.
    vm_destroy(vm);
    return 0;
}
