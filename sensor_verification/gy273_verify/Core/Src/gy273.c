/**
  ******************************************************************************
  * @file           : gy273.c
  * @brief          : GY273 Interface
  *                   This file contains the functions used to configure and
  *                   read data from GY273.
  ******************************************************************************
  */

#include "gy273.h"

// private defines
#define GYADDR (0x1E << 1)
#define GYREG_WHOAMI 0x10 // ID reg A

I2C_HandleTypeDef* myhi2c;

/** Writing to registers */
static void gy_reg_write(uint8_t reg, uint8_t value, uint32_t timeout) {
	HAL_I2C_Mem_Write(myhi2c, GYADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, timeout);
}

/** Reading registers */
static void gy_reg_read(uint8_t reg, uint8_t* pBuff, uint16_t nBytes, uint32_t timeout) {
	HAL_I2C_Mem_Read(myhi2c, reg, I2C_MEMADD_SIZE_8BIT, pBuff, nBytes, timeout);
}

/**	Initializing GY
 * 	INPUT:
 * 		hi2c - pointer to i2c handle
 * */
void gy_init(I2C_HandleTypeDef* hi2c) {
	myhi2c = hi2c;
	uint32_t timeout = 100;

	// check device status
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, GYADDR, 1, timeout);
	if (connectStatus == HAL_OK) {
		// read whoami
		uint8_t whoami;
		gy_reg_read(GYREG_WHOAMI, &whoami, 1, timeout);
		if (whoami == 0x48) {
			// connected!, do smt
		} else {
			// invalid, do smt
			return;
		}
	} else {
		// do smt
		return;
	}

	/** CONFIGURE GY */
}
