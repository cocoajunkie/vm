#include <IOKit/IONotifier.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include "VMService.h"
#include "Log.h"
#include "VMX.h"

static IOReturn sleep_wake_handler(void * target, void * refCon, UInt32 messageType, IOService * provider, void * messageArgument, vm_size_t argSize) {
	if (messageType == kIOMessageSystemWillSleep) {
		LOG("sleep_wake_handler: system will sleep");
		vmx_disable_on_all_host_cpus();
	}
	acknowledgeSleepWakeNotification(refCon);
	return kIOReturnSuccess;
}


#define super IOService

OSDefineMetaClassAndStructors(VMService, IOService)

bool VMService::start(IOService* provider) {
	LOG("VMService start");
	if (!vmx_init())
		return false;
	if (!super::start(provider))
		return false;
	sleep_wake_notifier = registerPrioritySleepWakeInterest(sleep_wake_handler, this);
	LOG("VMService register");
	registerService();
	return true;
}

void VMService::stop(IOService* provider) {
	LOG("VMService stop");
	if (sleep_wake_notifier)
		sleep_wake_notifier->remove();
	vmx_cleanup();
	super::stop(provider);
}
