#pragma once
#include <IOKit/IOUserClient.h>

struct VM;
struct Registers;

class VMUserClient : public IOUserClient {
OSDeclareDefaultStructors(VMUserClient)
public:
    bool initWithTask(task_t owningTask, void * securityToken, UInt32 type, OSDictionary * properties);	
	IOExternalMethod* getTargetAndMethodForIndex(IOService** target, UInt32 index);
	IOReturn clientClose();
	
	IOReturn vm_new(int num_cpus);
	IOReturn cpu_map_registers(int cpu, Registers** registers);
	IOReturn cpu_run(int cpu);

private:
	VM* vm;
	task_t owningTask;
};
