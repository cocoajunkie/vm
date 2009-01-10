#include "VM.h"
#include "Log.h"
#include "CPU.h"

VM* vm_new(int num_cpus) {
	if (num_cpus <= 0) {
		LOG_ERROR("vm_new: num_cpus <= 0");
		return NULL;		
	}
	
	VM* vm = (VM*)IOMalloc(sizeof(VM));
	if (!vm) {
		LOG_ERROR("vm_new: can't allocate VM");
		return NULL;
	}
	memset(vm, 0, sizeof(VM));
	
	vm->num_cpus = num_cpus;
	vm->cpus = (CPU**)IOMalloc(sizeof(CPU*) * num_cpus);
	if (!vm->cpus) {
		LOG_ERROR("vm_new: can't allocate cpus[]");
		vm_delete(vm);
		return NULL;
	}
	memset(vm->cpus, 0, sizeof(sizeof(CPU*) * num_cpus));
	for (int i = 0; i < vm->num_cpus; i++) {
		CPU* cpu = cpu_new();
		if (!cpu) {
			vm_delete(vm);
			return NULL;
		}
		vm->cpus[i] = cpu;
	}

	LOG("created VM");
	return vm;
}

void vm_delete(VM* vm) {
	if (vm->cpus) {
		for (int i = 0; i < vm->num_cpus; i++) {
			if (vm->cpus[i])
				cpu_delete(vm->cpus[i]);			
		}
		IOFree(vm->cpus, sizeof(CPU*) * vm->num_cpus);
	}
	IOFree(vm, sizeof(VM));
	LOG("destroyed VM");
}
