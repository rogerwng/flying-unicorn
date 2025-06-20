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
float hcsr04_readDistance();

/** Checking for hanging echo, if hanging, stops IT */
uint8_t hcsr04_hangCheck();

/** Check if sensor ready for next measurement (60ms have passed since last trigger and we are not waiting)*/
uint8_t hcsr04_readyCheck();
