#pragma once

struct CPU;

struct VM {
	int num_cpus;
	CPU** cpus;
};

VM* vm_new(int num_cpus);
void vm_delete(VM* vm);
