#pragma once

bool vmx_init();
void vmx_cleanup();
bool vmx_enable(int host_cpu);
void vmx_disable_on_all_host_cpus();
void vmx_disable_on_host_cpu();
bool vmx_is_enabled();
