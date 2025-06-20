/**
  ******************************************************************************
  * @file           : hcsr04.c
  * @brief          : HCSR04 Interface
  *                   This file contains the functions used to configure and
  *                   read data from HC-SR04.
  ******************************************************************************
  */

#include "hcsr04.h"

#define HCSR04_CAPTURE_TIMEOUT_US 10000UL // 10ms
#define HCSR04_CAPTURE_CYCLE_TIME_US 60000UL // 60ms

// wait flag set when sensor is triggered and we are waiting for echo to arrive/finish
volatile static uint8_t waitFlag = 0;
volatile static uint32_t t0;
// capture flag set when interrupt detects falling edge (end of echo signal)
volatile static uint8_t captureFlag = 0;
volatile static uint32_t t1;
volatile static uint32_t t2;
// measurement data
volatile static float distance;
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
static uint32_t calculateTime(uint32_t time1, uint32_t time2);

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
		clkFreq *= 2;
	}

	// start continuous timer
	HAL_TIM_Base_Start(myhtim);
}

/**	Delay function for microsecond delays using timer
 * 	INPUTS:
 * 		t - time in microseconds
 * */
static void delay_us(uint16_t t) {
	uint32_t timeStart = __HAL_TIM_GET_COUNTER(myhtim);
	uint32_t timeDiff = 0.0;

	while (timeDiff < t) {
		uint32_t timeCurr = __HAL_TIM_GET_COUNTER(myhtim);
		timeDiff = calculateTime(timeStart, timeCurr);
	}

}

/** Function to calculate time from tick differences
 * 	INPUTS:
 * 		time1 - first tick
 * 		time2 - second tick
 * 	OUTPUTS:
 * 		time in us
 */
static uint32_t calculateTime(uint32_t time1, uint32_t time2) {
	uint32_t diff;
	if (time2 >= time1) {
		diff = time2 - time1;
	} else {
		diff = (__HAL_TIM_GET_AUTORELOAD(myhtim) - time2 + time1 + 1); // handle overflow
	}

	// avoid 32 bit overflow
	uint64_t temp = (uint64_t)diff * (timerPrescaler + 1) * 1000000ULL;
	uint32_t time_us = (uint32_t)(temp / clkFreq);

	return time_us;
}

/**	Trigger sensor and enable interrupt
 * */
void hcsr04_trigger() {
	// dont trigger if we are still waiting for last trigger/wait/echo cycle
	if (waitFlag) {
		return;
	}

	// pull trigger high, then low
	HAL_GPIO_WritePin(myGPIO, triggerPin, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(myGPIO, triggerPin, GPIO_PIN_RESET);

	// enable input capture interrupt
	HAL_TIM_IC_Start_IT(myhtim, timChannel);

	// set wait flag
	__disable_irq();
	waitFlag = 1;
	__enable_irq();

	// save start time
	t0 = __HAL_TIM_GET_COUNTER(myhtim);
}

/**	IC Interrupt Routine
 * */
void hcsr04_echo_IT() {
	// 1st capture (rising edge) - start of echo
	if (!captureFlag) {
		//serialPrint("Rising edge echo IT\r\n");
		t1 = HAL_TIM_ReadCapturedValue(myhtim, timChannel);

		__HAL_TIM_SET_CAPTUREPOLARITY(myhtim, timChannel, TIM_INPUTCHANNELPOLARITY_FALLING);

		captureFlag = 1;

	} else if (captureFlag) {
		//serialPrint("Falling edge echo IT, capture complete.\r\n");
		// 2nd capture (falling edge) - echo complete
		t2 = HAL_TIM_ReadCapturedValue(myhtim, timChannel);

		HAL_TIM_IC_Stop_IT(myhtim, timChannel);
		__HAL_TIM_ENABLE(myhtim); // omg!!! - Stop_IT disables counter, need to re-enable

		// convert tick counts to micro-seconds
		uint32_t time_us = calculateTime(t1, t2);
		float d = (float)time_us / 58.0;

		// update distance and waitFlag
		__disable_irq();
		distance = d;
		waitFlag = 0;
		__enable_irq();

		captureFlag = 0;
		__HAL_TIM_SET_CAPTUREPOLARITY(myhtim, timChannel, TIM_INPUTCHANNELPOLARITY_RISING);
	}
}

/** Checking for hanging echo, if hanging, stops IT
 * 	OUTPUT:
 * 		0 (not hanging), 1 (hanging, IT stopped)
 */
uint8_t hcsr04_hangCheck() {
	if (waitFlag) {
		uint32_t t =__HAL_TIM_GET_COUNTER(myhtim);

		uint32_t time_us = calculateTime(t0, t);
		if (time_us >= HCSR04_CAPTURE_TIMEOUT_US) {
			captureFlag = 0;
			waitFlag = 0;
			HAL_TIM_IC_Stop_IT(myhtim, timChannel);
			__HAL_TIM_SET_CAPTUREPOLARITY(myhtim, timChannel, TIM_INPUTCHANNELPOLARITY_RISING);

			return 1;
		}
	}

	return 0;
}

/** Get latest distance measurement
 * 	OUTPUT:
 * 		distance
 */
float hcsr04_readDistance() {
	float t;

	__disable_irq();
	t = distance;
	__enable_irq();

	return t;
}

/** Check if sensor ready for next measurement (60ms have passed since last trigger and we are not waiting)
 * 	OUTPUT:
 * 		0 (not ready) or 1 (ready)
 */
uint8_t hcsr04_readyCheck() {
	// if waiting for echo, not ready
	if (waitFlag) {
		return 0;
		serialPrint("Not ready, wait flag");
	}

	// not waiting for echo, how long since last trigger
	uint32_t t = __HAL_TIM_GET_COUNTER(myhtim);
	uint32_t time_us = calculateTime(t0, t);
	// if enough time passed, we are ready to trigger again
	if (time_us >= HCSR04_CAPTURE_CYCLE_TIME_US) {
		return 1;
	}

	return 0;
}





