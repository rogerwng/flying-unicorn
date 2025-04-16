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
#define GYREG_CONFIGA 0x00
#define GYREG_CONFIGB 0x01
#define GYREG_MODE 0x02
#define GYREG_DATAOUTPUTX 0x03 // output regs from 0x03-0x08 (6 regs MSB+LSB alternating for 3x16-bit 2's comp integers)

#define GYCONVERSION_LSBTOG 1090.0f // for 1.3 Ga gain, the LSB/Gauss is 1090
#define GYCONVERSION_GTOuT 100.0f // conversion constant from Gauss to microTesla

// bias
float gyBias[3] = {0.0, 0.0, 0.0};
float gyScale[3] = {1.0, 1.0, 1.0};

I2C_HandleTypeDef* myhi2c;

/** Writing to registers */
static void gy_reg_write(uint8_t reg, uint8_t value, uint32_t timeout) {
	HAL_I2C_Mem_Write(myhi2c, GYADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, timeout);
}

/** Reading registers */
static void gy_reg_read(uint8_t reg, uint8_t* pBuff, uint16_t nBytes, uint32_t timeout) {
	HAL_I2C_Mem_Read(myhi2c, GYADDR, reg, I2C_MEMADD_SIZE_8BIT, pBuff, nBytes, timeout);
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
		serialPrint("Connecting to GY...HAL_OK.\r\n");
		// read whoami
		uint8_t whoami;
		gy_reg_read(GYREG_WHOAMI, &whoami, 1, timeout);
		if (whoami == 0x48) {
			// connected!, do smt
			serialPrint("Connecting to GY...whoami verified.\r\n");
		} else {
			// invalid, do smt
			serialPrint("Connecting to GY...whoami failed, exiting GY_INIT.\r\n");
			return;
		}
	} else {
		// do smt
		serialPrint("Connecting to GY...failed, exiting GY_INIT.\r\n");
		return;
	}

	/** CONFIGURE GY */

	// CONFIGA: 0b 0 [reserved] 11 [8 avg samples per output] 110 [75 Hz sample rate] 00 [normal measurement config] -> 0x78
	gy_reg_write(GYREG_CONFIGA, 0x78, timeout);
	// CONFIGB: 0b 001 [gain of 1.3Ga (default)] 00000 -> 0x20
	gy_reg_write(GYREG_CONFIGB, 0x20, timeout);
	// MODE: 0b 000000 [disable hi-speed i2c since we are using fast] 00 [continuous measurement mode] -> 0x00
	gy_reg_write(GYREG_MODE, 0x00, timeout);
}

/**	READING RAW DATA
 * 	INPUTS:
 * 		pBuff - pointer to data buffer
 * */
static void gy_readRawData(uint16_t* pBuff) {
	uint32_t timeout = 100;
	uint8_t rawBuff[6];
	gy_reg_read(GYREG_DATAOUTPUTX, rawBuff, 6, timeout);

	pBuff[0] = ((uint16_t)rawBuff[0] << 8) | (uint16_t)rawBuff[1];
	pBuff[1] = ((uint16_t)rawBuff[2] << 8) | (uint16_t)rawBuff[3];
	pBuff[2] = ((uint16_t)rawBuff[4] << 8) | (uint16_t)rawBuff[5];
}

/**	Converting raw outputs to real units
 * 	INPUTS:
 * 		pMagBuffer - pointer to float array where output data will be stored
 * 		pRawBuffer - pointer to uint16 array where raw data stored
 * */
static void gy_convertRawData(float* pMagBuffer, uint16_t* pRawBuffer) {
	pMagBuffer[0] = ((float)(int16_t)pRawBuffer[0] / GYCONVERSION_LSBTOG * GYCONVERSION_GTOuT - gyBias[0]) * gyScale[0];
	pMagBuffer[1] = ((float)(int16_t)pRawBuffer[1] / GYCONVERSION_LSBTOG * GYCONVERSION_GTOuT - gyBias[1]) * gyScale[1];
	pMagBuffer[2] = ((float)(int16_t)pRawBuffer[2] / GYCONVERSION_LSBTOG * GYCONVERSION_GTOuT - gyBias[2]) * gyScale[2];
}

/**	Reading data in float (uT)
 * 	INPUTS:
 * 		pMagBuffer - pointer to float array where data will be stored
 */
void gy_readData(float* pMagBuffer) {
	uint16_t rawBuffer[3];
	gy_readRawData(rawBuffer);

	gy_convertRawData(pMagBuffer, rawBuffer);
}

/**	Calibrating magnetometer
 */
void gy_calibrateBias() {
	serialPrint("Calibrating GY...collecting measurements.\r\n");

	// collect data from all orientations, just store min and max readings along each axis
	uint32_t totalCalibTime = 5000; // 5 sec for full orientation
	uint32_t delay = 50; // 50 ms between each reading
	uint32_t numReadings = totalCalibTime / delay;

	float mag[3];
	float mins[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float maxs[3] = {FLT_MIN, FLT_MIN, FLT_MIN};
	for (int i = 0; i < numReadings + 3; i++) { // ignore first 3 readings -> 3*delay warmup time
		HAL_Delay(delay);
		gy_readData(mag);
		if (i < 3) { continue; };

		for (int j = 0; j < 3; j++) {
			mins[j] = fminf(mins[j], mag[j]);
			maxs[j] = fmaxf(maxs[j], mag[j]);
		}
	}

	float avgScale = 0.0;
	for (int i = 0; i < 3; i++) {
		// hard iron offset
		gyBias[i] = (maxs[i] + mins[i]) / 2.0;

		// soft iron scaling
		gyScale[i] = (maxs[i] - mins[i]) / 2.0;
		avgScale += gyScale[i] / 3.0;
	}

	// final soft iron scale
	gyScale[0] = avgScale / gyScale[0];
	gyScale[1] = avgScale / gyScale[1];
	gyScale[2] = avgScale / gyScale[2];

	char buff[128];
	snprintf(buff, sizeof(buff), "Computed offset: X=%.3f, Y=%.3f, Z=%.3f, scaling factor: X=%.3f, Y=%.3f, Z=%.3f\r\n", gyBias[0], gyBias[1], gyBias[2], gyScale[0], gyScale[1], gyScale[2]);
	serialPrint(buff);
}
