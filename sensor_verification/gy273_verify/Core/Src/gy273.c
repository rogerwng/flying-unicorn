/**
  ******************************************************************************
  * @file           : gy273.c
  * @brief          : GY273 Interface
  *                   This file contains the functions used to configure and
  *                   read data from GY273 using QMC chip.
  ******************************************************************************
  */

#include <gy273.h>

// private defines
#define GYADDR (0x0D << 1)
#define GYREG_WHOAMI 0x0D // ID reg A
#define GYREG_CONFIG1 0x09
#define GYREG_DATAOUTPUTX 0x00 // output regs from 0x00-0x05 (6 regs LSB+MSB alternating for 3x16-bit 2's comp integers)

#define GYCONVERSION_LSBTOG 12000.0f // for +-2G full scale range, LSB/Gauss = 12000
#define GYCONVERSION_GTOuT 100.0f // conversion constant from Gauss to microTesla

// bias
static float gyBias[3] = {0.0, 0.0, 0.0};
static float gyScale[3] = {1.0, 1.0, 1.0};

static I2C_HandleTypeDef* myhi2c;

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
	HAL_Delay(300);

	myhi2c = hi2c;
	uint32_t timeout = 100;

	// check device status
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, GYADDR, 1, timeout);
	if (connectStatus == HAL_OK) {
		serialPrint("Connecting to GY...HAL_OK.\r\n");
		// read whoami
		uint8_t whoami;
		gy_reg_read(GYREG_WHOAMI, &whoami, 1, timeout);
		if (whoami == 0xFF) {
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

	// CONFIG 1: 01 [256 OSR] 00 [2G FSR] 01 [50Hz] 01 [continuous mode] -> 0x45
	gy_reg_write(GYREG_CONFIG1, 0x45, timeout);
}

/**	READING RAW DATA
 * 	INPUTS:
 * 		pBuff - pointer to data buffer
 * */
static void gy_readRawData(uint16_t* pBuff) {
	uint32_t timeout = 100;
	uint8_t rawBuff[6];
	gy_reg_read(GYREG_DATAOUTPUTX, rawBuff, 6, timeout);

	pBuff[0] = ((uint16_t)rawBuff[1] << 8) | (uint16_t)rawBuff[0];
	pBuff[1] = ((uint16_t)rawBuff[3] << 8) | (uint16_t)rawBuff[2];
	pBuff[2] = ((uint16_t)rawBuff[5] << 8) | (uint16_t)rawBuff[4];
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
	float maxs[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
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

	// final soft iron scale: this formula simplifies the full matrix calibration by only considering diagonals (axis-wise scaling) and not rotations
	gyScale[0] = avgScale / gyScale[0];
	gyScale[1] = avgScale / gyScale[1];
	gyScale[2] = avgScale / gyScale[2];

	char buff[128];
	snprintf(buff, sizeof(buff), "Computed offset: X=%.3f, Y=%.3f, Z=%.3f, scaling factor: X=%.3f, Y=%.3f, Z=%.3f\r\n", gyBias[0], gyBias[1], gyBias[2], gyScale[0], gyScale[1], gyScale[2]);
	serialPrint(buff);
}
