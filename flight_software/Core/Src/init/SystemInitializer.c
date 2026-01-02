/**
 * Initializes all Flight Software Components and Tasks
 */

#include "SystemInitializer.h"
#include "Logger.h"

#include "cmsis_os2.h"

// Logger tag
static const char TAG[] = "SYSINIT";

// Task handles
static osThreadId_t loggerTaskHandle;

bool SystemInitializer_Init() {
    // Initialize logger first
    if (!Logger_Init()) 
        return false;
    LOG_DIRECT(TAG, "Logger initialized");

    LOG_DIRECT(TAG, "System Initialized");
    return true;
}

void SystemInitializer_Start() {
    // Create logger task
    osThreadAttr_t logAttr = {
        .name = "Logger",
        .stack_size = 3072,
        .priority = osPriorityLow
    };
    loggerTaskHandle = osThreadNew(LoggerTask, NULL, &logAttr);

    // Start kernel
    osKernelStart();
}

void SystemInitializer_Stop() {
    // not implemented
    return;
}