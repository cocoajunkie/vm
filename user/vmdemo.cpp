#include "VMAPI.h"
#include <exception>
#include <stdio.h>

int main() {
	try {
		int num_cpus = 1;
		vm_new(num_cpus);
		int cpu = 0;
		Registers* regs = cpu_map_registers(cpu);
		cpu_run(cpu);
	}
	catch (std::exception& e) {
		printf("error: %s\n", e.what());
	}
	vm_cleanup();
	return 0;
}
