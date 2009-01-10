#pragma once

#define CPUID(eax, ebx, ecx, edx) \
	asm("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)  : "a"(eax), "b"(ebx), "c"(ecx), "d"(edx))

#define RDMSR(index, hi, lo) \
	asm("rdmsr" : "=d"(hi), "=a"(lo) : "c"(index))

#define WRMSR(index, hi, lo) \
	asm volatile("wrmsr" : : "c"(index), "d"(hi), "a"(lo))

#define READ_CR(cr_num, dest) \
	asm("movl %%cr" #cr_num ", %0" : "=r"(dest))
	
#define WRITE_CR(cr_num, src) \
	asm volatile("movl %0, %%cr" #cr_num : : "r"(src))	
	