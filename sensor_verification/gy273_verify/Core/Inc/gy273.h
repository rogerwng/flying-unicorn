/**
  ******************************************************************************
  * @file           : gy273.h
  * @brief          : Header for gy273.c file.
  *                   This file contains the defines of the GY273 w/ QMC chip interface.
  ******************************************************************************
  */

#include "main.h"

/* Initializing the sensor */
void gy_init(I2C_HandleTypeDef *hi2c);

/** Calibrating sensor */
void gy_calibrateBias();

/* Reading data converted to real units */
void gy_readData(float* pMagBuffer);
