#pragma once

#include "Registers.h"

// IMPORTANT: modify methods[] in kernel/VMUserClient.h when changing the enum
enum {
	VM_NEW, // vm_new(int num_cpus)
	CPU_MAP_REGISTERS, // cpu_map_registers(int cpu, void** registers)
	CPU_RUN, // cpu_run(int cpu)
};

void vm_new(int num_cpus);
void vm_cleanup();
Registers* cpu_map_registers(int cpu);
void cpu_run(int cpu);
