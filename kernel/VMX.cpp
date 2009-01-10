#include "VMX.h"
#include "Types.h"
#include "Log.h"
#include "Host.h"
#include "Assembly.h"
#include "KernelExports.h"

#define VMX_FAIL_INVALID -1
#define VMX_FAIL_VALID -2
#define VMX_SUCCESS 0

struct VMXRegion {
	void* vaddr;
	u64 paddr;
};

static int num_vmxon_regions = 0;
static VMXRegion* vmxon_regions = NULL;

struct VMXRegister {
	u32 lo;
	u32 hi;
};

#define IA32_VMX_BASIC_MSR 0x480
static struct VMXBasicRegister {
	u32 revision;
	u16 region_size : 13;
	u8 reserved : 3;
	u8 region_address_limit : 1;
	u8 dual_monitor_support : 1;
	u8 memory_type : 4;
	u8 in_out_reporting : 1;
	u16 reserved2 : 9;
} basic;

#define IA32_VMX_CR0_FIXED0_MSR 0x486 
#define IA32_VMX_CR0_FIXED1_MSR 0x487
#define IA32_VMX_CR4_FIXED0_MSR 0x488
#define IA32_VMX_CR4_FIXED1_MSR 0x489
#define IA32_FEATURE_CONTROL_MSR 0x3a

#define KERNEL_CS		0x8
#define KERNEL64_CS     0x80
static inline void enter_64bit_mode(void) {
	__asm__ __volatile__ (
		".byte   0xea    /* far jump longmode */	\n\t"
		".long   1f					\n\t"
		".word   %P0					\n\t"
		".code64					\n\t"
		"1:"
		:: "i" (KERNEL64_CS)
	);
}
static inline void enter_compat_mode(void) {
	asm(
		"ljmp    *4f					\n\t"
	"4:							\n\t"
		".long   5f					\n\t"
		".word   %P0					\n\t"
		".code32					\n\t"
	"5:"
		:: "i" (KERNEL_CS)
	);
}

bool vmx_is_suppported() {
	u32 eax, ebx, ecx, edx;
	eax = 1;
	CPUID(eax, ebx, ecx, edx);
	return (ecx & 0x20); // ecx[5] = 1 when vmx is supported
}

bool vmx_init() {
	if (!vmx_is_suppported()) {
		LOG_ERROR("vmx_init: VMX is not supported");
		return false;		
	}
	
	RDMSR(IA32_VMX_BASIC_MSR, ((VMXRegister*)&basic)->hi, ((VMXRegister*)&basic)->lo);	
	num_vmxon_regions = host_get_num_cpus();
	LOG("vmx_init: allocating %d VMXON regions of %d bytes (rev# 0x%x)", num_vmxon_regions, basic.region_size, basic.revision);
	vmxon_regions = (VMXRegion*)IOMalloc(num_vmxon_regions * sizeof(VMXRegion));
	if (!vmxon_regions) {
		LOG_ERROR("vmx_init: can't allocate vmxon_regions");
		return false;
	}
	memset(vmxon_regions, 0, num_vmxon_regions * sizeof(VMXRegion));
	for (int i = 0; i < num_vmxon_regions; i++) {
		vmxon_regions[i].vaddr = IOMallocContiguous(basic.region_size, 4096, (IOPhysicalAddress*)&vmxon_regions[i].paddr);
		if (!vmxon_regions[i].vaddr) {
			LOG_ERROR("vmx_init: can't allocate vmxon region %d", i);
			vmx_cleanup();
			return false;
		}
		LOG("vmx_init: allocated vmxon region %d, paddr = %x", i, vmxon_regions[i].paddr);
	}
	
	return true;
}

void vmx_cleanup() {
	if (vmxon_regions) {
		vmx_disable_on_all_host_cpus();
		for (int i = 0; i < num_vmxon_regions; i++) {
			if (vmxon_regions[i].vaddr)
				IOFreeContiguous(vmxon_regions[i].vaddr, basic.region_size);
		}
		IOFree(vmxon_regions, num_vmxon_regions * sizeof(VMXRegion));
		vmxon_regions = NULL;
	}
}

bool vmx_enable(int host_cpu) {
	if (vmx_is_enabled())
		return true;

	if (host_cpu < 0 || host_cpu >= num_vmxon_regions) {
		LOG_ERROR("vmx_enable(host_cpu=%d): host_cpu doesn't index vmxon region", host_cpu);
		return false;
	}		

	u32 feature_control_hi, feature_control_lo;	
	RDMSR(IA32_FEATURE_CONTROL_MSR, feature_control_hi, feature_control_lo);
	//LOG("IA32_FEATURE_CONTROL_MSR: %x", feature_control_lo);	
	if (!(feature_control_lo & (1 << 2))) {
		// VMXON is disabled outside of SMX operation, enable it if lock bit is not set
		if (feature_control_lo & 1) {
			LOG_ERROR("vmx_enable(host_cpu=%d): can't enable VMXON, lock bit is set", host_cpu);
			return false;
		}
		feature_control_lo |= (1 << 2);
		WRMSR(IA32_FEATURE_CONTROL_MSR, feature_control_hi, feature_control_lo);
	}
	if (!(feature_control_lo & 1)) {
		// set lock bit
		feature_control_lo |= 1;
		WRMSR(IA32_FEATURE_CONTROL_MSR, feature_control_hi, feature_control_lo);		
	}
	
	// set fixed bits 31:0 to either 1 or 0 in CR0 & CR4
	// bits 63:32 of CR0 and CR4 are reserved and must be written with zeros
	
	u32 cr0_fixed0_hi, cr0_fixed0_lo; 
	RDMSR(IA32_VMX_CR0_FIXED0_MSR, cr0_fixed0_hi, cr0_fixed0_lo);
	u32 cr0_fixed1_hi, cr0_fixed1_lo; 
	RDMSR(IA32_VMX_CR0_FIXED1_MSR, cr0_fixed1_hi, cr0_fixed1_lo);
	//LOG("CR0_FIXED0: %x CR0_FIXED1: %x", cr0_fixed0_lo, cr0_fixed1_lo);
	u64 cr0 = 0;
	READ_CR(0, cr0);
	//LOG("cr0 = %x", cr0);
	cr0 = (cr0 | cr0_fixed0_lo) & cr0_fixed1_lo;
	//LOG("new cr0 = %x", cr0);
	WRITE_CR(0, cr0);
	
	u32 cr4_fixed0_hi, cr4_fixed0_lo; 
	RDMSR(IA32_VMX_CR4_FIXED0_MSR, cr4_fixed0_hi, cr4_fixed0_lo);
	u32 cr4_fixed1_hi, cr4_fixed1_lo; 
	RDMSR(IA32_VMX_CR4_FIXED1_MSR, cr4_fixed1_hi, cr4_fixed1_lo);
	//LOG("CR4_FIXED0: %x CR4_FIXED1: %x", cr4_fixed0_lo, cr4_fixed1_lo);
	u64 cr4 = 0;
	READ_CR(4, cr4);
	//LOG("cr4 = %x", cr4);
	cr4 = (cr4 | cr4_fixed0_lo) & cr4_fixed1_lo;
	//LOG("new cr4 = %x", cr4);
	WRITE_CR(4, cr4);
	
	memset(vmxon_regions[host_cpu].vaddr, 0, basic.region_size);
	*(u32*)(vmxon_regions[host_cpu].vaddr) = basic.revision;	
	
	u64* vmxon_region_paddr_ptr = &vmxon_regions[host_cpu].paddr;
	int result = VMX_SUCCESS;
	enter_64bit_mode();
	asm volatile("vmxon %3  \n\t" 
				 "cmovcl %2, %0"
				: "=&r"(result)
				: "0"(VMX_SUCCESS),
				  "r"(VMX_FAIL_INVALID),
				  "m"(*vmxon_region_paddr_ptr)				
				: "memory", "cc");
	enter_compat_mode();
	if (result != VMX_SUCCESS) {
		LOG_ERROR("vmx_enable(host_cpu=%d): vmxon failed", host_cpu);
		cr4 &= ~(1 << 13);
		WRITE_CR(4, cr4);
		return false;
	}
	
	LOG("vmx_enable(host_cpu=%d): vmxon %x", host_cpu, *vmxon_region_paddr_ptr);				

	return true;
}

void vmx_disable_on_all_host_cpus() {
	mp_rendezvous(NULL, (void (*)(void *))vmx_disable_on_host_cpu, NULL, NULL);	
}

void vmx_disable_on_host_cpu() {
	if (!vmx_is_enabled())
		return;

	int host_cpu = cpu_number();
	int result = VMX_SUCCESS;
	enter_64bit_mode();
	asm volatile("vmxoff \n\t" 
				 "cmovcl %2, %0 \n\t"
				 "cmovzl %3, %0"
				: "=&r"(result)
				: "0"(VMX_SUCCESS),
				  "r"(VMX_FAIL_INVALID),
				  "r"(VMX_FAIL_VALID)				
				: "memory", "cc");
	enter_compat_mode();
	if (result != VMX_SUCCESS) {
		LOG_ERROR("vmx_disable_on_host_cpu %d: vmxoff failed", host_cpu);
		return;
	}

	u64 cr4 = 0;
	READ_CR(4, cr4);
	cr4 &= ~(1 << 13);
	WRITE_CR(4, cr4);
	
	LOG("vmx_disable_on_host_cpu %d", host_cpu);
}

bool vmx_is_enabled() {
	u64 cr4 = 0;
	READ_CR(4, cr4);
	return (cr4 & (1 << 13)) ? true : false;
}
