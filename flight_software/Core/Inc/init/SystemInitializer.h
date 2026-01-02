/**
 * Initializes all Flight Software Components and Tasks
 */

#pragma once

#include <stdbool.h>

/**
 * @brief Initializes all FSW Components
 * @returns True on success, False otherwise
 */
bool SystemInitializer_Init();

/**
 * @brief Starts the FSW by creating all tasks and starting the RTOS kernel
 */
void SystemInitializer_Start();

/**
 * @brief Stops the FSW and cleans up remaining resources
 */
void SystemInitializer_Stop();