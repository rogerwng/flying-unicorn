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

static I2C_HandleTypeDef* myhi2c;

// compensation parameters read from sensor
static uint16_t compT[3];
static uint16_t compP[9];
static int32_t t_fine;

// relative altitude baseline
static float pBase = 0.0;

/* Writing to registers */
static void bmp_reg_write(uint8_t reg, uint8_t value, uint32_t timeout) {
	HAL_I2C_Mem_Write(myhi2c, BMPADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, timeout);
}
/* Reading registers */
static void bmp_reg_read(uint8_t reg, uint8_t* pBuff, uint16_t nBytes, uint32_t timeout) {
	HAL_I2C_Mem_Read(myhi2c, BMPADDR, reg, I2C_MEMADD_SIZE_8BIT, pBuff, nBytes, timeout);
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

/**	Converting raw temperature reading into float in Kelvin
 * 	INPUT:
 * 		t - raw temperature reading
 * 	OUTPUT:
 * 		temperature reading in degrees Celsius
 * */
static float convertRawTemp(int32_t tRaw) {
	// define compensation variables
	float dig_T1 = (float)compT[0];
	float dig_T2 = (float)(int16_t)compT[1];
	float dig_T3 = (float)(int16_t)compT[2];

	// compensation formula edited from datasheet
	float var1, var2, T;
	var1 = ((((float)tRaw)/16384.0) - (dig_T1/1024.0)) * dig_T2;
	var2 = ((((float)tRaw)/131072.0) - (dig_T1/8192.0)) * ((((float)tRaw)/131072.0) - dig_T1/8192.0) * dig_T3;
	t_fine = (int32_t)(var1 + var2);
	T = (var1 + var2) / 5120.0;
	return T;
}

/**	Converting raw pressure reading into float in Pascals, note that temperature must be converted first (to init t_fine)
 * 	INPUT:
 * 		p - raw pressure reading
 * 	OUTPUT:
 * 		pressure reading in Pascals
 * */
static float convertRawPressure(int32_t pRaw) {
	// define compensation variables
	float dig_P1 = (float)compP[0];
	float dig_P2 = (float)(int16_t)compP[1];
	float dig_P3 = (float)(int16_t)compP[2];
	float dig_P4 = (float)(int16_t)compP[3];
	float dig_P5 = (float)(int16_t)compP[4];
	float dig_P6 = (float)(int16_t)compP[5];
	float dig_P7 = (float)(int16_t)compP[6];
	float dig_P8 = (float)(int16_t)compP[7];
	float dig_P9 = (float)(int16_t)compP[8];

	// compensation formula edited from datasheet
	float var1, var2, p;
	var1 = ((float)t_fine)/2.0 - 64000.0;
	var2 = var1 * var1 * dig_P6 / 32768.0;
	var2 = var2 + var1 * dig_P5 * 2.0;
	var2 = (var2/4.0) + (dig_P4 * 65536.0);
	var1 = (dig_P3 * var1 * var1 / 524288.0 + (dig_P2 * var1)) / 524288.0;
	var1 = (1.0 + var1 / 32768.0) * dig_P1;
	if (var1 == 0.0) {
		return 0; // avoid exception division by zero
	}
	p = 1048576.0 - (float)pRaw;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = dig_P9 * p * p / 2147483648.0;
	var2 = p * dig_P8 / 32768.0;
	p = p + (var1 + var2 + dig_P7) / 16.0;
	return p;
}

/**	Use pressure and temperature to calculate altitude
 * 	INPUT:
 * 		p - pressure reading in Pascals
 * 	OUTPUT:
 * 		altitude in meters
 * */
static float calculateAltitude(float p) {
	float altitude;

	// define constants
	float T0 = 288.15; // temp @ sea level
	float P0 = 101325.0; // air pressure @ sea level
	float L = 0.0065; // temp lapse rate
	float R = 8.31447; // univ gas constant
	float g = 9.80665; // acceleration to gravity
	float M = 0.0289644; // molar mass of air

	float var1, var2, var3;
	var1 = T0/L;
	var2 = p/P0;
	var3 = (R * L) / (g * M);
	altitude = var1 * (powf(var2, var3) - 1);

	return altitude;
}

/**	Reading final absolute altitude measurement from BMP
 * 	INPUTS:
 * 		pAlt - pointer to altitude measurement buffer
 * */
void bmp_readData(float* pAlt) {
	uint32_t rawPressure;
	uint32_t rawTemperature;
	bmp_readRawData(&rawPressure, &rawTemperature);

	// raw data compensation, temperature must be done first
	convertRawTemp((int32_t)rawTemperature);
	float pressure = convertRawPressure((int32_t)rawPressure);

	/**char buff[128];
	snprintf(buff, sizeof(buff), "RawPress=%lu, RawTemp=%lu, Press=%.3f, Temp=%.3f\r\n", (unsigned long)rawPressure, (unsigned long)rawTemperature, pressure, temp);
	serialPrint(buff);*/

	// altitude calculation
	float altitude = calculateAltitude(pressure);

	// update altitude buffer
	*pAlt = altitude;
}

/**	Reading final relative altitude measurement from BMP
 * 	INPUTS:
 * 		pAlt - pointer to altitude measurement buffer
 * */
void bmp_readDataRelative(float* pAlt) {
	uint32_t rawPressure;
	uint32_t rawTemperature;
	bmp_readRawData(&rawPressure, &rawTemperature);

	// raw data compensation, temperature must be done first
	convertRawTemp((int32_t)rawTemperature);
	float pressure = convertRawPressure((int32_t)rawPressure);

	/**char buff[128];
	snprintf(buff, sizeof(buff), "RawPress=%lu, RawTemp=%lu, Press=%.3f, Temp=%.3f\r\n", (unsigned long)rawPressure, (unsigned long)rawTemperature, pressure, temp);
	serialPrint(buff);*/

	// altitude calculation
	float altitude = calculateAltitude(pressure);

	// update altitude buffer
	*pAlt = altitude - pBase;
}

/** Calibrating Relative altitude at Power On */
static void calibratePBase() {
	float mean_altitude = 0.0;
	float altitude;

	for (int i = 0; i < 10; i++) {
		HAL_Delay(30);
		bmp_readData(&altitude);
		mean_altitude += altitude / 10.0;
	}

	pBase = mean_altitude;
}

/**	Initializing BMP
 * 	INPUT:
 * 		hi2c - pointer to i2c handle
 * */
void bmp_init(I2C_HandleTypeDef *hi2c) {
	HAL_Delay(500);
	myhi2c = hi2c;
	uint32_t timeout = 100;

	// check device connection
	HAL_StatusTypeDef connectStatus = HAL_I2C_IsDeviceReady(hi2c, BMPADDR, 1, timeout);	if (connectStatus == HAL_OK) {
		serialPrint("Connecting to BMP...HAL_OK.\r\n");
		// do smt
		uint8_t whoami;
		bmp_reg_read(BMPREG_WHOAMI, &whoami, 1, timeout);
		if (whoami == 0x58) {
			// bmp successful connection
			serialPrint("Connecting to BMP...whoami verified.\r\n");
		} else {
			serialPrint("Connecting to BMP...whoami failed, exiting BMP_INIT.\r\n");
			return;
		}
	} else {
		// do smt
		serialPrint("Connecting to BMP...failed, exiting BMP_INIT.\r\n");
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
		compT[i] = ((uint16_t)compTBuffer[i*2]) | ((uint16_t)compTBuffer[i*2+1] << 8);
		compP[i] = ((uint16_t)compPBuffer[i*2]) | ((uint16_t)compPBuffer[i*2+1] << 8);
	}
	for (int i = 3; i < 9; i++) {
		compP[i] = ((uint16_t)compPBuffer[i*2]) | ((uint16_t)compPBuffer[i*2+1] << 8);
	}

	// calibrate relative altitude
	calibratePBase();
}
