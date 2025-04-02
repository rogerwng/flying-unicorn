/**
  ******************************************************************************
  * @file           : bmp280.h
  * @brief          : Header for bmp280.c file.
  *                   This file contains the defines of the BMP280 interface.
  ******************************************************************************
  */

#include "main.h"

/* Initializing the sensor */
void bmp_init(I2C_HandleTypeDef *hi2c);

/* Reading data converted to real units */
void bmp_readData(float* pAlt);
