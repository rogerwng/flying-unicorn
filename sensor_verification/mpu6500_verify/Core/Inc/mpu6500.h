/**
  ******************************************************************************
  * @file           : mpu6500.h
  * @brief          : Header for mpu6500.c file.
  *                   This file contains the defines of the MPU6500 interface.
  ******************************************************************************
  */

#include "main.h"

#define MPUADDR (0x68 << 1) // ADO pulled low

/* Initializing the sensor (configuring accelerometer and gyro) */
void mpu_init(I2C_HandleTypeDef *hi2c);

/* Reading data */
void mpu_readData();
