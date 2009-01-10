#include "VMUserClient.h"
#include "Log.h"
#include "VM.h"
#include "CPU.h"

#define super IOUserClient

OSDefineMetaClassAndStructors(VMUserClient, IOUserClient)

bool VMUserClient::initWithTask(task_t owningTask, void* securityToken, UInt32 type, OSDictionary* properties) {
	this->owningTask = owningTask;
	return super::initWithTask(owningTask, securityToken, type, properties);
}

// IMPORTANT: modify user/VMAPI.h when changing contents of methods[]
static IOExternalMethod methods[] = {
	{
		NULL,
		(IOMethod)&VMUserClient::vm_new,
		kIOUCScalarIScalarO,
		1,
		0
	},
	{
		NULL,
		(IOMethod)&VMUserClient::cpu_map_registers,
		kIOUCScalarIScalarO,
		1,
		1
	},
	{
		NULL,
		(IOMethod)&VMUserClient::cpu_run,
		kIOUCScalarIScalarO,
		1,
		0
	},
};

IOExternalMethod* VMUserClient::getTargetAndMethodForIndex(IOService** target, UInt32 index) {
	int num_methods = sizeof(methods) / sizeof(IOExternalMethod);	
	if (index >= num_methods)
		return NULL;
	*target = this;
	return &methods[index];
}

IOReturn VMUserClient::vm_new(int num_cpus) {
	if (vm)
		return kIOReturnError;

	vm = ::vm_new(num_cpus);
	if (!vm)
		return kIOReturnError;

	return kIOReturnSuccess;	
}

IOReturn VMUserClient::clientClose() {
	if (vm) {
		::vm_delete(vm);
		vm = NULL;
	}
	return kIOReturnSuccess;
}


IOReturn VMUserClient::cpu_map_registers(int cpu, Registers** registers) {
	if (!vm || cpu < 0 || cpu >= vm->num_cpus)
		return kIOReturnError;
	*registers = ::cpu_map_registers(vm->cpus[cpu], owningTask);
	return kIOReturnSuccess;
}

IOReturn VMUserClient::cpu_run(int cpu) {
	if (!vm || cpu < 0 || cpu >= vm->num_cpus)
		return kIOReturnError;
	::cpu_run(vm->cpus[cpu]);
	return kIOReturnSuccess;
}
