/**
  ******************************************************************************
  * @file           : mpu6500.c
  * @brief          : MPU6500 Interface
  *                   This file contains the functions used to configure and
  *                   read data from MPU6500.
  ******************************************************************************
  */

#include "mpu6500.h"



/* Writing to registers */
static void mpu_reg_write(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value, uint32_t timeout) {
	HAL_I2C_Mem_Write(hi2c, MPUADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, timeout);
}

/**
 * INPUTS:
 * 		hi2c - pointer to i2c handle
 * 		addr - 7bit address of i2c device (should alr be left shifted by 1)
 * */
void mpu_init(I2C_HandleTypeDef *hi2c) {
	uint32_t timeout = 100; // 100ms
	// check device connection
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, MPUADDR, 1, timeout);
	if (connectStatus == HAL_OK) {
		// verify MPU connection
		uint8_t whoami = 0x00;
		HAL_I2C_Mem_Read(hi2c, MPUADDR, MPUREG_WHOAMI, I2C_MEMADD_SIZE_8BIT, &whoami, 1, timeout);
		if (whoami == 0x70) {
			// success: do smt
		} else {
			// failed: do smt
		}
	} else {
		// do smt
	}


	/** CONFIGURE MPU */
	// register PWR_MGMT_1, set bit 7 for device reset, on reset the accel/gyro/temp enabled by default, set bit 3 after reset to disable temp
	mpu_reg_write(hi2c, MPUREG_PWR_MGMT_1, 0x80, timeout);  // device reset
	mpu_reg_write(hi2c, MPUREG_PWR_MGMT_1, 0x08, timeout);  // disable temp sensor
	// register SMPLRT_DIV sets DLPF sample rate
	mpu_reg_write(hi2c, MPUREG_SMPLRT_DIV, 0x07, timeout);  // for 125 Hz sample rate
	// register CONFIG configures DLPF
	mpu_reg_write(hi2c, MPUREG_CONFIG, 0x03, timeout); // 41Hz bandwidth, 5.9ms delay
	// set full scale range of gyro and accl, and set accl dlpf
	mpu_reg_write(hi2c, MPUREG_GYROCONFIG, 0x18, timeout); // 2000 dps, disables gyro dlpf bypass
	mpu_reg_write(hi2c, MPUREG_ACCLCONFIG, 0x10, timeout); // 8g
	mpu_reg_write(hi2c, MPUREG_ACCLCONFIG2, 0x03, timeout); // 41Hz, 11.8ms
}

void mpu_readData() {

}
