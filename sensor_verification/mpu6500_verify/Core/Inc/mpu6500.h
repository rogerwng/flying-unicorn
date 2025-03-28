/**
  ******************************************************************************
  * @file           : mpu6500.h
  * @brief          : Header for mpu6500.c file.
  *                   This file contains the defines of the MPU6500 interface.
  ******************************************************************************
  */

#include "main.h"

#define MPUADDR (0x68 << 1) // ADO pulled low
#define MPUREG_WHOAMI 0x75
#define MPUREG_PWR_MGMT_1 0x6B
#define MPUREG_SMPLRT_DIV 0x19
#define MPUREG_CONFIG 0x1A
#define MPUREG_GYROCONFIG 0x1B
#define MPUREG_ACCLCONFIG 0x1C
#define MPUREG_ACCLCONFIG2 0x1D

/* Initializing the sensor (configuring accelerometer and gyro) */
void mpu_init(I2C_HandleTypeDef *hi2c);

/* Reading data */
void mpu_readData();
