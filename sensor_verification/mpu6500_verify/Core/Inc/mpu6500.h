/**
  ******************************************************************************
  * @file           : mpu6500.h
  * @brief          : Header for mpu6500.c file.
  *                   This file contains the defines of the MPU6500 interface.
  ******************************************************************************
  */

#include "main.h"

/* Initializing the sensor (configuring accelerometer and gyro) */
void mpu_init(I2C_HandleTypeDef *hi2c);

/** Calibrating sensor bias */
void mpu_calibrateBias();

/* Reading data converted to real units */
void mpu_readData(float* pAccBuffer, float* pGyroBuffer);
