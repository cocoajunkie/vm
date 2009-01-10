#include <libkern/libkern.h>
#include <libkern/sysctl.h>

int host_get_num_cpus() {
	int num_cpus = 0;
	size_t size = sizeof(num_cpus);
	sysctlbyname("hw.activecpu", &num_cpus, &size, NULL, 0);
	return num_cpus;
}
