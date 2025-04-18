/**
  ******************************************************************************
  * @file           : neo8m.c
  * @brief          : NEO-8M Interface
  *                   This file contains the functions used to configure and
  *                   read data from NEO-8M.
  ******************************************************************************
  */

#include "neo8m.h"

static UART_HandleTypeDef* myhuart;

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
		} else if (reading == 1 && idx >= sizeof(rawBuff) - 1) {
			// error in sentence reading, restart
			idx = 0;
			reading = 0;
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
	if (idx >= buffSize) {
		serialPrint("neo8m_readLine() error: length of sentence longer than buffer. Returning.\r\n");
		return;
	}

	for (int i = 0; i < idx; i++) {
		buff[i] = (char)rawBuff[i];
	}
	// add null terminator
	buff[idx] = '\0';
}

/**	Parsing a GGA sentence
 * 	INPUT:
 * 		buff - pointer to string containing sentence, $ expected at index 0
 * 		buffSize - length of buffer
 * 		gpsBuff - pointer to float array where data will be stored
 * 	OUTPUT:
 * 		uint8_t status - 2 if valid line, 1 if valid line but not enough sats, 0 otherwise
 * */
uint8_t neo8m_parseSentence(char* buff, uint32_t buffSize, float* gpsBuff) {
	// check GGA sentence
	if (strncmp(buff, "$GPGGA", 6) != 0 && strncmp(buff, "$GNGGA", 6) != 0) {
		serialPrint("Parsing sentence error...not GGA, exiting.\r\n");
		return 0;
	}

	// valid GGA sentence, iterate thru characters starting from idx = 6, which should be comma
	float latitude = 0.0, longitude = 0.0, altitude = 0.0;
	int sat_check = 1; // true if good num sats

	int term_counter = 0;
	int term_idx = 0;
	char term[16];
	for (int i = 6; i < buffSize; i++) {
		if (buff[i] == ',') {
			term[term_idx] = '\0';
			// start of new term, check last term - term 1 (time), term 2 (latitude), term 3 (N/S), term 4 (longitude), term 5 (E/W), term 6 (fix quality), term 7 (num satelites), term 9 (altitude)
			if (term_counter == 2) {
				float rawLatitude = atof(term);
				latitude = (int)(rawLatitude / 100);
				latitude += (rawLatitude - latitude * 100) / 60.0f;
			} else if (term_counter == 4) {
				float rawLongitude = atof(term);
				longitude = (int)(rawLongitude / 100);
				longitude += (rawLongitude - longitude * 100) / 60.0f;
			} else if (term_counter == 9) {
				altitude = atof(term);
				break; // no more useful terms after altitude
			} else if (term_counter == 3 && term[0] == 'S') {
				// flip latitude sign
				latitude *= -1;
			} else if (term_counter == 5 && term[0] == 'W') {
				// flip longitude sign
				longitude *= -1;
			} else if (term_counter == 6 && term[0] == '0' ) {
				// fix quality invalid
				serialPrint("Parsing sentence error...invalid fix quality, exiting.\r\n");
				return 0;
			} else if (term_counter == 7 && atof(term) < 5) {
				// check if num satelites above threshold
				serialPrint("Parsing sentence warning...too little number of satelites.\r\n");
				sat_check = 0;
			}
			term_counter++;
			term_idx = 0;
			continue;
		}

		// no new term, write to term buffer
		if (term_idx < 15) {
			term[term_idx] = buff[i];
			term_idx++;
		}
	}

	// successful read, update buffers
	gpsBuff[0] = latitude;
	gpsBuff[1] = longitude;
	gpsBuff[2] = altitude;
	if (sat_check) {
		return 2;
	} else {
		return 1;
	}
}

/**	Reading a line of valid data output in blocking mode
 * 	INPUT:
 * 		gpsData - pointer to float array where lat,long, and alt data will be stored
 * */
void neo8m_readData(float* gpsData) {
	char buff[128];
	neo8m_readLine(buff, 128);
	while (neo8m_parseSentence(buff, 128, gpsData) != 2) {
		// keep trying new sentences
		HAL_Delay(50);
		neo8m_readLine(buff, 128);
	}

}


