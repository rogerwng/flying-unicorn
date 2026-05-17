#pragma once

#include "main.h"

// ===========================
// HARDWARE HANDLES
// ===========================

extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi2;

// ===========================
// HANDLE ALIASES
// ===========================

inline constexpr UART_HandleTypeDef* LOGGER_HUART = &huart1;
inline constexpr SPI_HandleTypeDef* IMU_SPI = &hspi2;