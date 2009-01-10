#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOLocks.h>

#include "CPU.h"
#include "Log.h"
#include "Registers.h"
#include "VMX.h"
#include "KernelExports.h"

CPU* cpu_new() {
	CPU* cpu = (CPU*)IOMalloc(sizeof(CPU));
	if (!cpu) {
		LOG_ERROR("cpu_new: can't allocate CPU");
		return NULL;
	}
	memset(cpu, 0, sizeof(CPU));
	
	cpu->regsMemoryDescriptor = IOBufferMemoryDescriptor::withOptions(kIOMemoryKernelUserShared, sizeof(Registers));	
	if (!cpu->regsMemoryDescriptor) {
		LOG_ERROR("cpu_new: can't allocate registers");
		cpu_delete(cpu);
		return NULL;
	}
	cpu->regs = (Registers*)cpu->regsMemoryDescriptor->getBytesNoCopy();
	memset(cpu->regs, 0, sizeof(Registers));

	LOG("created CPU");
	return cpu;
}

void cpu_delete(CPU* cpu) {
	if (cpu->regsMapping)
		cpu->regsMapping->release();
	if (cpu->regsMemoryDescriptor)
		cpu->regsMemoryDescriptor->release();
	IOFree(cpu, sizeof(CPU));
	LOG("destroyed CPU");
}

Registers* cpu_map_registers(CPU* cpu, task_t task) {
	if (!cpu->regsMapping) {
		cpu->regsMapping = cpu->regsMemoryDescriptor->createMappingInTask(task, 0, kIOMapAnywhere);
		if (!cpu->regsMapping) {
			LOG_ERROR("cpu_map_registers: can't create mapping");
			return NULL;
		}
		LOG("mapped CPU registers");
	}
	return (Registers*)cpu->regsMapping->getVirtualAddress();
}

void cpu_run(CPU* cpu) {	
	IOSimpleLock* lock = IOSimpleLockAlloc();
	if (!lock) {
		LOG_ERROR("cpu_run: can't allocate simple lock");
		return;
	}
	IOSimpleLockLock(lock);
	
	int host_cpu = cpu_number();
	
	// vmxon if not already turned on
	if (!vmx_enable(host_cpu)) {
		goto cpu_run_exit;
	}

	/* TODO:
		init vmcs
			vmclear cpu's vmcs
			vmptrld cpu's vmcs
			init exit/entry/exec control fields
			vmclear cpu's vmcs
	*/
	
	/* TODO: 
		vmptrld cpu's vmcs
		update vmcs
			update host state
			update guest state		
		load guest registers
		vmlaunch
		vm exit:
			vmclear to flush cached info to vmcs in case next vm entry is on different host cpu			
	*/
	LOG("running CPU");

cpu_run_exit:	
	IOSimpleLockUnlock(lock);
	IOSimpleLockFree(lock);
}
