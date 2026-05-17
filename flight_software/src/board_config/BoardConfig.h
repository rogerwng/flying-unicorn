#pragma once

#include "main.h"

// ===========================
// HARDWARE HANDLES
// ===========================

extern UART_HandleTypeDef huart1;

// ===========================
// HANDLE ALIASES
// ===========================

inline constexpr UART_HandleTypeDef* LOGGER_HUART = &huart1;