/**
  ******************************************************************************
  * @file           : hcsr04.h
  * @brief          : Header for hcsr04.c file.
  *                   This file contains the defines of the HC-SR04 interface.
  ******************************************************************************
  */

#include "main.h"

/**	Initializing sensor by saving TIM and GPIO handles */
void hcsr04_init(TIM_HandleTypeDef* htim, uint32_t channel, GPIO_TypeDef* GPIOx, uint16_t triggerPin);

/** Trigger */
void hcsr04_trigger();
/** Interupt routine */
void hcsr04_echo_IT();

/** Read latest distance measurement */
float hcsr04_readData();
