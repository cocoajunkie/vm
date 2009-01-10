#pragma once
#include <IOKit/IOService.h>

class IONotifier;

class VMService : public IOService {
OSDeclareDefaultStructors(VMService)
public:
	bool start(IOService* provider);
	void stop(IOService* provider);

private:
	IONotifier* sleep_wake_notifier;	
};
