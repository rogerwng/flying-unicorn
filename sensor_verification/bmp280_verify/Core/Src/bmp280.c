/**
  ******************************************************************************
  * @file           : bmp280.c
  * @brief          : BMP280 Interface
  *                   This file contains the functions used to configure and
  *                   read data from BMP280.
  ******************************************************************************
  */

#include "bmp280.h"

// private defines
#define BMPADDR (0x76 << 1) // SDO pulled low
#define BMPREG_WHOAMI 0xD0
#define BMPREG_CONFIG 0xF5
#define BMPREG_CTRL_MEAS 0xF4
#define BMPREG_DIGT1 0x88 // temp compensation variables 0x88-0x8D (6 bytes)
#define BMPREG_DIGP1 0x8E // pres compensation variables 0x8E-0x9F (18 bytes)
#define BMPREG_MEAS 0xF7 // measurements should be done as burst read from 0xF7-0xFC (6 bytes), temp and pressure are both 20-bit

I2C_HandleTypeDef* myhi2c;
uint16_t compT[3];
uint16_t compP[9];

/* Writing to registers */
static void bmp_reg_write(uint8_t reg, uint8_t value, uint32_t timeout) {
	HAL_I2C_Mem_Write(myhi2c, BMPADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, timeout);
}
/* Reading registers */
static void bmp_reg_read(uint8_t reg, uint8_t* pBuff, uint16_t nBytes, uint32_t timeout) {
	HAL_I2C_Mem_Read(myhi2c, BMPADDR, reg, I2C_MEMADD_SIZE_8BIT, pBuff, nBytes, timeout);
}

/**	Initializing BMP
 * 	INPUT:
 * 		hi2c - pointer to i2c handle
 * */
void bmp_init(I2C_HandleTypeDef *hi2c) {
	myhi2c = hi2c;
	uint32_t timeout = 100;

	// check device connection
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, BMPADDR, 1, timeout);
	if (connectStatus == HAL_OK) {
		// do smt
		uint8_t whoami;
		bmp_reg_read(BMPREG_WHOAMI, &whoami, 1, 100);
		if (whoami == 0x58) {
			// bmp successful connection
		} else {
			return;
		}
	} else {
		// do smt
		return;
	}

	/** CONFIGURE BMP */
	// config register sets sample rate and filter: 001 [62.5 ms sample rate] 100 [x16 IIR filter] 00 [I2C enable] -> 0x30
	bmp_reg_write(BMPREG_CONFIG, 0x30, 100);
	// ctrl_meas register sets oversampling and power mode: 010 [x2 temp oversample] 101 [x16 pressure oversample] 11 [normal mode] -> 0x57
	bmp_reg_write(BMPREG_CTRL_MEAS, 0x57, 100);

	/** SAVE BMP COMPENSATION VARIABLES */
	uint8_t compTBuffer[6];
	uint8_t compPBuffer[18];
	bmp_reg_read(BMPREG_DIGT1, compTBuffer, 6, 100);
	bmp_reg_read(BMPREG_DIGP1, compPBuffer, 18, 100);
	// combine single bytes into 2-byte words
	for (int i = 0; i < 3; i++) {
		compT[i] = ((uint16_t)compTBuffer[i*2] << 8) | (uint16_t)compTBuffer[i*2+1];
		compP[i] = ((uint16_t)compPBuffer[i*2] << 8) | (uint16_t)compPBuffer[i*2+1];
	}
	for (int i = 3; i < 9; i++) {
		compP[i] = ((uint16_t)compPBuffer[i*2] << 8) | (uint16_t)compPBuffer[i*2+1];
	}
}

/**	Reading raw measurements from BMP
 * 	INPUTS:
 * 		pPress - pointer to pressure measurement buffer
 * 		pTemp - pointer to temperature measurement buffer
 * */
static void bmp_readRawData(uint32_t* pPress, uint32_t* pTemp) {
	uint8_t buffer[6];
	bmp_reg_read(BMPREG_MEAS, buffer, 6, 100); // burst read measurements
	// measurements are 20bits, store in 32bit buffer
	*pPress = ((uint32_t)buffer[0] << 12) | ((uint32_t)buffer[1] << 4) | ((uint32_t)buffer[2] >> 4);
	*pTemp = ((uint32_t)buffer[3] << 12) | ((uint32_t)buffer[4] << 4) | ((uint32_t)buffer[5] >> 4);
}

/**	Converting raw pressure reading into float in Pascals
 * 	INPUT:
 * 		p - raw pressure reading
 * 	OUTPUT:
 * 		pressure reading in Pascals
 * */
static float convertRawPressure(uint32_t p) {

}

/**	Converting raw temperature reading into float in Kelvin
 * 	INPUT:
 * 		t - raw temperature reading
 * 	OUTPUT:
 * 		temperature reading in Kelvin
 * */
static float convertRawTemp(uint32_t t) {

}

/**	Use pressure and temperature to calculate altitude
 * 	INPUT:
 * 		p - pressure reading in Pascals
 * 		t - temperature reading in Kelvin
 * 	OUTPUT:
 * 		altitude in meters
 * */
static float calculateAltitude(float p, float t) {

}

/**	Reading final altitude measurement from BMP
 * 	INPUTS:
 * 		pAlt - pointer to altitude measurement buffer
 * */
void bmp_readData(float* pAlt) {
	uint32_t pressure;
	uint32_t temperature;
	bmp_readRawData(&pressure, &temperature);

	// convert raw data into real units

	// perform compensation and altitude calculation

	// update altitude buffer
	*pAlt = 0.0;
}
