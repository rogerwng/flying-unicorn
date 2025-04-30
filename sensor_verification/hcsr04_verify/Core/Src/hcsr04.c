/**
  ******************************************************************************
  * @file           : hcsr04.c
  * @brief          : HCSR04 Interface
  *                   This file contains the functions used to configure and
  *                   read data from HC-SR04.
  ******************************************************************************
  */

#include "hcsr04.h"

// capture flag set when interrupt detects falling edge (end of echo signal)
static uint8_t captureFlag = 0;
// measurement data
static float distance;

// peripheral handles
static TIM_HandleTypeDef* myhtim;
static uint32_t timChannel;
static GPIO_TypeDef* myGPIO;
static uint16_t triggerPin;

/** static function declarations */
static void delay_us(uint16_t t);

/**	Initializing sensor by saving TIM and GPIO handles
 * 	INPUTS:
 * 		GPIOx - GPIO port for trigger and echo pins
 * 		tPin - trigger pin number
 * 		ePin - echo pin number
 * */
void hcsr04_init(TIM_HandleTypeDef* htim, uint32_t channel, GPIO_TypeDef* GPIOx, uint16_t tPin) {
	myhtim = htim;
	timChannel = channel;
	myGPIO = GPIOx;
	triggerPin = tPin;
}

/**	Delay function for microsecond delays using timer
 * 	INPUTS:
 * 		t - time in microseconds
 * */
static void delay_us(uint16_t t) {
	// clear timer counter
	__HAL_TIM_SET_COUNTER(myhtim, 0);
	// start timer
	HAL_TIM_Base_Start(myhtim);

	// count to delay (each count is 1us)
	while (__HAL_TIM_GET_COUNTER(myhtim) < t);
	// stop counting
	HAL_TIM_Base_Stop(myhtim);
}

/**	Trigger sensor and enable interrupt
 * */
void hcsr04_trigger() {
	// pull trigger high, then low
	HAL_GPIO_WritePin(myGPIO, triggerPin, GPIO_PIN_SET);
	delay_ms(10);
	HAL_GPIO_WritePin(myGPIO, triggerPin, GPIO_PIN_RESET);

	// enable input capture interrupt
	HAL_TIM_IC_Start_IT(myhtim, timChannel);
}

/**	IC Interrupt Routine
 * */
void hcsr04_echo_IT() {

}
