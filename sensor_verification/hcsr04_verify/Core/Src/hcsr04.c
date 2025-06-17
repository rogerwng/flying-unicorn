/**
  ******************************************************************************
  * @file           : hcsr04.c
  * @brief          : HCSR04 Interface
  *                   This file contains the functions used to configure and
  *                   read data from HC-SR04.
  ******************************************************************************
  */

#include "hcsr04.h"

#define HCSR04_CAPTURE_TIMEOUT_US 600

// capture flag set when interrupt detects falling edge (end of echo signal)
static uint8_t captureFlag = 0;
static uint32_t t1;
static uint32_t t2;
// measurement data
static float distance;
// compensation constants
uint32_t timerPrescaler;
uint32_t clkFreq;

// peripheral handles
static TIM_HandleTypeDef* myhtim;
static uint32_t timChannel;
static GPIO_TypeDef* myGPIO;
static uint16_t triggerPin;

/** static function declarations */
static void delay_us(uint16_t t);
static float calculateTime(uint32_t time1, uint32_t time2);

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

	timerPrescaler = myhtim->Init.Prescaler;
	clkFreq = HAL_RCC_GetPCLK1Freq();

	// timer clock doubled when APB1 prescaler > 1
	if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
		clfFreq *= 2;
	}
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

/** Function to calculate time from tick differences
 * 	INPUTS:
 * 		time1 - first tick
 * 		time2 - second tick
 * 	OUTPUTS:
 * 		time in us
 */
static float calculateTime(uint32_t time1, uint32_t time2) {
	uint32_t diff;
	if (time2 >= time1) {
		diff = time2 - time1;
	} else {
		diff = (__HAL_TIM_GET_AUTORELOAD(myhtim) - time2 + time1 + 1); // handle overflow
	}

	float time_us = (diff * (timerPrescaler + 1)) / clkFreq;
}

/**	Trigger sensor and enable interrupt
 * */
void hcsr04_trigger() {
	// pull trigger high, then low
	HAL_GPIO_WritePin(myGPIO, triggerPin, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(myGPIO, triggerPin, GPIO_PIN_RESET);

	// enable input capture interrupt
	HAL_TIM_IC_Start_IT(myhtim, timChannel);
}

/**	IC Interrupt Routine
 * */
void hcsr04_echo_IT() {
	// 1st capture (rising edge) - start of echo
	if (!captureFlag) {
		t1 = HAL_TIM_ReadCapturedValue(myhtim, timChannel);

		__HAL_TIM_SET_CAPTUREPOLARITY(myhtim, timChannel, TIM_INPUTCHANNELPOLARITY_FALLING);

		captureFlag = 1;

	} else if (captureFlag) {
		// 2nd capture (falling edge) - echo complete
		t2 = HAL_TIM_ReadCapturedValue(myhtim, timChannel);

		HAL_TIM_IC_Stop_IT(myhtim, timChannel);

		// convert tick counts to seconds
		float time_us = calculateTime(t1, t2);

		// update distance
		__disable_irq();
		distance = time_us/58.0f;
		__enable_irq();

		captureFlag = 0;
		__HAL_TIM_SET_CAPTUREPOLARITY(myhtim, timChannel, TIM_INPUTCHANNELPOLARITY_RISING);
	}
}

/** Checking for hanging echo, if hanging, stops IT
 * 	OUTPUT:
 * 		0 (not hanging), 1 (hanging)
 */
uint8_t hcsr04_hangCheck() {
	if (captureFlag) {
		uint32_t t = HAL_TIM_ReadCapturedValue(myhtim, timChannel);

		float time_us = calculateTime(t1, t);
		if (time_us >= HCSR04_CAPTURE_TIMEOUT_US) {
			captureFlag = 0;
			HAL_TIM_IC_Stop_IT(myhtim, timChannel);
			__HAL_TIM_SET_CAPTURE_POLARITY(myhtim, timChannel, TIM_INPUTCHANNELPOLARITY_RISING);
		}
	}
}
