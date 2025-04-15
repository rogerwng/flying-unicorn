/**
  ******************************************************************************
  * @file           : mpu6500.c
  * @brief          : MPU6500 Interface
  *                   This file contains the functions used to configure and
  *                   read data from MPU6500.
  ******************************************************************************
  */

#include "mpu6500.h"

// private defines
#define MPUADDR (0x68 << 1) // ADO pulled low
// config registers
#define MPUREG_WHOAMI 0x75
#define MPUREG_PWR_MGMT_1 0x6B
#define MPUREG_SMPLRT_DIV 0x19
#define MPUREG_CONFIG 0x1A
#define MPUREG_GYROCONFIG 0x1B
#define MPUREG_ACCLCONFIG 0x1C
#define MPUREG_ACCLCONFIG2 0x1D
// measurement registers
#define MPUREG_ACC 0x3B // from 0x3b - 0x40
#define MPUREG_GYRO 0x43 // from 0x43 - 0x48
// scale factors for data conversion
#define ACCDATA_SCALE_FACTOR (8.0f * 9.81f / 32768.0f)  // for 8g full-scale, converted to m/s²
#define GYRODATA_SCALE_FACTOR (2000.0f / 32768.0f)  // for 2000 dps range, converted to degrees/second

I2C_HandleTypeDef* myhi2c;

/* Writing to registers */
static void mpu_reg_write(uint8_t reg, uint8_t value, uint32_t timeout) {
	HAL_I2C_Mem_Write(myhi2c, MPUADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, timeout);
}
/* Reading registers */
static void mpu_reg_read(uint8_t reg, uint8_t* pBuff, uint16_t nBytes, uint32_t timeout) {
	HAL_I2C_Mem_Read(myhi2c, MPUADDR, reg, I2C_MEMADD_SIZE_8BIT, pBuff, nBytes, timeout);
}

/** Initializing MPU:
 * 		check mpu connection, verify device, reset device, initialize sensor dplf and full range scale
 * 	INPUTS:
 * 		hi2c - pointer to i2c handle
 * */
void mpu_init(I2C_HandleTypeDef *hi2c) {
	myhi2c = hi2c;
	uint32_t timeout = 100; // 100ms

	// check device connection
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, MPUADDR, 3, timeout);
	if (connectStatus == HAL_OK) {
		serialPrint("Connecting to MPU...HAL_OK.\r\n");
		// verify MPU connection
		uint8_t whoami = 0x00;
		mpu_reg_read(MPUREG_WHOAMI, &whoami, 1, timeout);
		if (whoami == 0x70) {
			// success: do smt
			serialPrint("Reading MPU...whoami verified.\r\n");
		} else {
			// failed: do smt
			serialPrint("Reading MPU...whoami failed, exiting MPU_INIT.\r\n");
			return;
		}
	} else {
		// do smt
		serialPrint("Connecting to MPU...failed, exiting MPU_INIT.\r\n");
		return;
	}

	/** CONFIGURE MPU */
	// register PWR_MGMT_1, set bit 7 for device reset, on reset the accel/gyro/temp enabled by default, set bit 3 after reset to disable temp
	mpu_reg_write(MPUREG_PWR_MGMT_1, 0x80, timeout);  // device reset
	mpu_reg_write(MPUREG_PWR_MGMT_1, 0x08, timeout);  // disable temp sensor
	// register SMPLRT_DIV sets DLPF sample rate
	mpu_reg_write(MPUREG_SMPLRT_DIV, 0x07, timeout);  // for 125 Hz sample rate
	// register CONFIG configures DLPF
	mpu_reg_write(MPUREG_CONFIG, 0x03, timeout); // 41Hz bandwidth, 5.9ms delay
	// set full scale range of gyro and accl, and set accl dlpf
	mpu_reg_write(MPUREG_GYROCONFIG, 0x18, timeout); // 2000 dps, disables gyro dlpf bypass
	mpu_reg_write(MPUREG_ACCLCONFIG, 0x10, timeout); // 8g
	mpu_reg_write(MPUREG_ACCLCONFIG2, 0x03, timeout); // 41Hz, 11.8ms
}

/** Reading raw data from MPU
 * 	INPUTS:
 * 		pAccBuffer - pointer to beginning of data buffer for accelerometer, 2bytes for each axis, 3axis = 6byte buffer
 * 		pGyroBuffer - pointer to beginning of data buffer for gyrometer, 2bytes for each axis, 3axis = 6bytes
 * */
static void mpu_readRawData(uint16_t* pAccBuffer, uint16_t* pGyroBuffer) {
	uint32_t timeout = 100;
	uint8_t regBuffer[6];
	// read acc
	mpu_reg_read(MPUREG_ACC, regBuffer, 6, timeout);
	// store into buffer
	pAccBuffer[0] = ((uint16_t)regBuffer[0] << 8) | (uint16_t)regBuffer[1];
	pAccBuffer[1] = ((uint16_t)regBuffer[2] << 8) | (uint16_t)regBuffer[3];
	pAccBuffer[2] = ((uint16_t)regBuffer[4] << 8) | (uint16_t)regBuffer[5];
	// read gyro
	mpu_reg_read(MPUREG_GYRO, regBuffer, 6, timeout);
	// store into buffer
	pGyroBuffer[0] = ((uint16_t)regBuffer[0] << 8) | (uint16_t)regBuffer[1];
	pGyroBuffer[1] = ((uint16_t)regBuffer[2] << 8) | (uint16_t)regBuffer[3];
	pGyroBuffer[2] = ((uint16_t)regBuffer[4] << 8) | (uint16_t)regBuffer[5];
}

/**	Reading data from MPU in real units
 * 	INPUTS:
 *		pAccBuffer - pointer to buffer for acc data
 *		pGyroBuffer - pointer to buffer for gyro data
 * */
void mpu_readData(float* pAccBuffer, float* pGyroBuffer) {
	// get raw
	uint16_t rawAcc[3];
	uint16_t rawGyro[3];
	mpu_readRawData(rawAcc, rawGyro);

	// convert raw to m/s
	for (int i = 0; i < 3; i++) {
		pAccBuffer[i] = (int16_t)(rawAcc[i]) * ACCDATA_SCALE_FACTOR;
		pGyroBuffer[i] = (int16_t)(rawGyro[i]) * GYRODATA_SCALE_FACTOR;
	}
}
