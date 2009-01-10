#pragma once

extern "C" int cpu_number();

extern "C" void mp_rendezvous(void (*setup_func)(void *), void (*action_func)(void *), void (*teardown_func)(void *), void *arg);

