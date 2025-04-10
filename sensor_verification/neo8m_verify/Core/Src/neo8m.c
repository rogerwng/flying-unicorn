/**
  ******************************************************************************
  * @file           : neo8m.c
  * @brief          : NEO-8M Interface
  *                   This file contains the functions used to configure and
  *                   read data from NEO-8M.
  ******************************************************************************
  */

#include "neo8m.h"

UART_HandleTypeDef* myhuart;

/**	Helper function to compute checksums of NMEA commands
 * 	INPUT:
 * 		cmd - a string (array of chars so its a pointer)
 * 	OUTPUT:
 * 		checksum - the checksum in uint8
 * */
static uint8_t computeChecksum(char* cmd) {
	uint8_t checksum = 0;

	while (*cmd && *cmd != '*') {
		if (*cmd != '$') {
			checksum ^= (uint8_t)*cmd;
		}
		cmd++;
	}
	return checksum;
}

/** INITIALIZTING NEO8M
 * 	INPUTS:
 * 		huart - pointer to uart handle
 * */
void neo8m_init(UART_HandleTypeDef* huart) {
	myhuart = huart;

	/** Configuring output sentences */
	// using nmea commands to configure gps output sentences
	char* nmea_commands[] = {
			"$PUBX,40,GGA,1,0,0,0*00\r\n", // enables GGA on UART1
			"$PUBX,40,GLL,0,0,0,0*00\r\n", // disables GLL
			"$PUBX,40,GSA,0,0,0,0*00\r\n", // disables GSA
			"$PUBX,40,GSV,0,0,0,0*00\r\n", // disables GSV
			"$PUBX,40,RMC,0,0,0,0*00\r\n", // disables RMC
			"$PUBX,40,VTG,0,0,0,0*00\r\n", // disables VTG
			"$PUBX,40,TXT,0,0,0,0*00\r\n"  // disables TXT
	};

	// need to calculate checksum of each sentence and add it into string
	for (int i = 0; i < 7; i++) {
		uint8_t checksumHex = computeChecksum(nmea_commands[i]);
		char checksumString[3];
		sprintf(checksumString, "%02X", checksumHex);
		checksumString[2] = '\0';

		nmea_commands[i][21] = checksumString[0];
		nmea_commands[i][22] = checksumString[1];
	}

	// send the nmea commands over uart to configure neo8m
	for (int i = 0; i < 7; i++) {
		HAL_UART_Transmit(myhuart, (uint8_t*)nmea_commands[i], strlen(nmea_commands[i]), 100);
		HAL_Delay(100);
	}
}
