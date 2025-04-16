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

/**	Reading Single Line of NEO-8M data in blocking mode
 * 	INPUTS:
 * 		buff - pointer to string buffer
 * 		buffSize - size of buffer
 */
void neo8m_readLine(char* buff, uint32_t buffSize) {
	uint8_t rawBuff[128]; // buffer to store incoming chars
	uint8_t rxd; // read chars one at a time, store in raw buff
	uint8_t idx = 0; // keep track of current write-index in buffer
	uint8_t reading = 0; // set to true if we are reading sentence

	while (HAL_UART_Receive(myhuart, &rxd, 1, HAL_MAX_DELAY) == HAL_OK) { // continuously reads a byte of data while connection is good
		if (rxd == '$') { // we have read the sentence start
			rawBuff[0] = rxd;
			idx = 1;
			reading = 1;
			continue;
		} else if (reading == 1 && rxd == '\n') { // we have read end of sentence
			rawBuff[idx] = rxd;
			idx++;
			break;
		} else if (reading == 1) { // read sentence start, havent read end
			rawBuff[idx] = rxd;
			idx++;
			continue;
		}
	}

	// copy sentence to buffer
	if (idx > buffSize) {
		serialPrint("neo8m_readLine() error: length of sentence longer than buffer. Returning.\r\n");
		return;
	}

	for (int i = 0; i < idx; i++) {
		buff[i] = (char)rawBuff[i];
	}
}
