#include "VMAPI.h"
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdexcept>
#include <stdio.h>

static io_service_t vm_service = NULL;
static io_connect_t vm_user_client = NULL;

void vm_new(int num_cpus) {
	if (vm_user_client)
		return;

	CFMutableDictionaryRef services = IOServiceMatching("VMService");
	vm_service = IOServiceGetMatchingService(kIOMasterPortDefault, services);
	if (!vm_service)
		throw std::runtime_error("no VMService");
		
	if (IOServiceOpen(vm_service, mach_task_self(), 0, &vm_user_client) != KERN_SUCCESS) {
		IOObjectRelease(vm_service);
		vm_service = NULL;
		throw std::runtime_error("can't connect to VMService");
	}
	
	IOConnectMethodScalarIScalarO(vm_user_client, VM_NEW, 1, 0, num_cpus);
	printf("VMAPI: created a new VM\n");	
}

void vm_cleanup() {
	if (vm_user_client) {
		IOServiceClose(vm_user_client);
		vm_user_client = NULL;
	}
	if (vm_service) {
		IOObjectRelease(vm_service);
		vm_service = NULL;
	}
}

Registers* cpu_map_registers(int cpu) {
	Registers* registers = NULL;
	IOConnectMethodScalarIScalarO(vm_user_client, CPU_MAP_REGISTERS, 1, 1, cpu, &registers);	
	if (!registers)
		throw std::runtime_error("can't map registers");
	return registers;
}

void cpu_run(int cpu) {
	IOConnectMethodScalarIScalarO(vm_user_client, CPU_RUN, 1, 0, cpu);
}
