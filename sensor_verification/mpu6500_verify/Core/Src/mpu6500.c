/**
  ******************************************************************************
  * @file           : mpu6500.c
  * @brief          : MPU6500 Interface
  *                   This file contains the functions used to configure and
  *                   read data from MPU6500.
  ******************************************************************************
  */

#include "mpu6500.h"

/**
 * INPUTS:
 * 		hi2c - pointer to i2c handle
 * 		addr - 7bit address of i2c device (should alr be left shifted by 1)
 * */
void mpu_init(I2C_HandleTypeDef *hi2c) {
	uint32_t trials = 1; // try connection 1 time
	uint32_t timeout = 100; // 100ms
	// check device connection
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, MPUADDR, trials, timeout);
	if (connectStatus == HAL_OK) {
		// do smt
	} else {
		// do smt
	}

	// configure mpu

}

void mpu_readData() {

}
