#pragma once
#include <IOKit/IOLib.h>

#define LOG(format, ...) IOLog("vm: " format "\n", ##__VA_ARGS__) 

#define LOG_ERROR(format, ...) IOLog("vm: error: " format "\n", ##__VA_ARGS__) 
