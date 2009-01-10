#pragma once

#include <IOKit/IOTypes.h>

struct Registers;

class IOBufferMemoryDescriptor;
class IOMemoryMap;

struct CPU {
	IOBufferMemoryDescriptor* regsMemoryDescriptor;	
	Registers* regs;
	IOMemoryMap* regsMapping;
};

CPU* cpu_new();
void cpu_delete(CPU* cpu);
Registers* cpu_map_registers(CPU* cpu, task_t task);
void cpu_run(CPU* cpu);
