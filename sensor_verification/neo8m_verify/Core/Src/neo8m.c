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

// receive incoming interrupt data in circular buffer
#define GPS_BUFFER_LEN 256
#define GPS_TEMP_BUFFER_LEN 64
volatile static uint8_t gpsBuffer[GPS_BUFFER_LEN];
volatile static uint16_t gpsBufferWrite = 0;
volatile static uint16_t gpsBufferRead = 0;
volatile static uint8_t sentenceReadyFlag = 0;
// temp buffer to store single sentence
static char sentenceBuffer[GPS_TEMP_BUFFER_LEN];
// latest data information
static float gpsData[2];

/** Accessor function to get state information
 * 	INPUTS:
 *		buff - buffer to return information
 * */
void neo8m_getCurrentData(float* buff) {
	__disable_irq();
	buff[0] = gpsData[0];
	buff[1] = gpsData[1];
	__enable_irq();
}

/** Mutator function to update state information
 * 	INPUTS:
 * 		data - float array
 * 		nData - number of data fields to update, 2 or 3
 */
void neo8m_updateCurrentData(float* data) {
	__disable_irq();
	gpsData[0] = data[0];
	gpsData[1] = data[1];
	__enable_irq();
}

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

/**	Validate the checksum for a given sentence stored in a buffer
 * 	INPUT:
 * 		buff - buffer containing sentence, expected buff[0] == '$'
 * 		buffSize - size of buffer
 * 	OUTPUT:
 * 		0 or 1 if checksum valid
 * */
static uint8_t validateChecksum(char* buff, uint32_t buffSize) {
	// extract checksum from sentence
	uint16_t checksumIDX = 0; // store start of checksum
	for (int i = 0; i < buffSize; i++) {
		if (buff[i] == '*') {
			checksumIDX = i + 1;
			break;
		}
	}
	// check checksum out of bounds or no * found in sentence
	if (checksumIDX + 1 >= buffSize || checksumIDX == 0) {
		return 0;
	}

	// extract checksum, compare it
	char sentenceChecksum[3] = {buff[checksumIDX], buff[checksumIDX + 1], '\0'};
	uint8_t checksum = (uint8_t)strtol(sentenceChecksum, NULL, 16);
	uint8_t computedChecksum = computeChecksum(buff);

	if (checksum == computedChecksum) {
		return 1;
	} else {
		return 0;
	}
}

/** INITIALIZTING NEO8M
 * 	INPUTS:
 * 		huart - pointer to uart handle
 * */
void neo8m_init(UART_HandleTypeDef* huart) {
	myhuart = huart;

	gpsData[0] = 0;
	gpsData[1] = 0;

	/** Configuring output sentences */
	// using nmea commands to configure gps output sentences
	/*
	char* nmea_commands[] = {
			"$PUBX,40,GGA,1,0,0,0*33\r\n", // enables GGA on UART1
			"$PUBX,40,GLL,0,0,0,0*2F\r\n", // disables GLL
			"$PUBX,40,GSA,0,0,0,0*3A\r\n", // disables GSA
			"$PUBX,40,GSV,0,0,0,0*3B\r\n", // disables GSV
			"$PUBX,40,RMC,0,0,0,0*2E\r\n", // disables RMC
			"$PUBX,40,VTG,0,0,0,0*3F\r\n", // disables VTG
			"$PUBX,40,TXT,0,0,0,0*34\r\n"  // disables TX
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
		HAL_Delay(500);
	}
	*/
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

/** Parse sentence prototypes */
static uint8_t parseGGA(char* buff, uint32_t buffSize, float* gpsBuff);
static uint8_t parseGLL(char* buff, uint32_t buffSize, float* gpsBuff);
static uint8_t parseRMC(char* buff, uint32_t buffSize, float* gpsBuff);


/** Parsing NMEA sentences */
uint8_t neo8m_parseSentence(char* buff, uint32_t buffSize, float* gpsBuff) {
	// validate NMEA checksum
	if (validateChecksum(buff, buffSize) == 0) {
		//serialPrint("neo8m_parseSentence: Parsing sentence error...invalid checksum, exiting.\r\n");
		return 0;
	}

	// see what type of sentence we have
	if (strncmp(buff, "$GPGGA", 6) == 0 || strncmp(buff, "$GNGGA", 6) == 0) {
		return parseGGA(buff, buffSize, gpsBuff);
	} else if (strncmp(buff, "$GPGLL", 6) == 0 || strncmp(buff, "$GNGLL", 6) == 0) {
		return parseGLL(buff, buffSize, gpsBuff);
	} else if (strncmp(buff, "$GPRMC", 6) == 0 || strncmp(buff, "$GNRMC", 6) == 0) {
		return parseRMC(buff, buffSize, gpsBuff);
	} else {
		//serialPrint("neo8m_parseSentence: unknown or irrelevant NMEA sentence...exiting.\r\n");
		return 0;
	}
}

/**	Parsing a GGA sentence
 * 	INPUT:
 * 		buff - pointer to string containing sentence, $ expected at index 0
 * 		buffSize - length of buffer
 * 		gpsBuff - pointer to float array where data will be stored
 * 	OUTPUT:
 * 		uint8_t status - 3 if valid line, 1 if valid line but not enough sats, 0 otherwise
 * */
static uint8_t parseGGA(char* buff, uint32_t buffSize, float* gpsBuff) {
	//serialPrint("parseGGA sentence\r\n");

	// valid GGA sentence, iterate thru characters starting from idx = 6, which should be comma
	float latitude = 0.0, longitude = 0.0;//, altitude = 0.0;
	int sat_check = 1; // true if good num sats

	// using strtok_r
	int tokenctr = 0;
	char* tokenptr;
	char* token = strtok_r(buff, ",", &tokenptr);
	while (token != NULL) {
		if (tokenctr == 2) {
			if (token[0] == '\0') return 0;
			// latitude
			float rawLatitude = atof(token);
			latitude = (int)(rawLatitude / 100.0f);
			latitude += (rawLatitude - latitude * 100) / 60.0f;
		} else if (tokenctr == 3) {
			if (token[0] == '\0') return 0;
			// N/S
			if (token[0] == 'S') latitude *= -1;

		} else if (tokenctr == 4) {
			if (token[0] == '\0') return 0;
			// longitude
			float rawLongitude = atof(token);
			longitude = (int)(rawLongitude / 100.0f);
			longitude += (rawLongitude - longitude * 100) / 60.0f;
		} else if (tokenctr == 5) {
			if (token[0] == '\0') return 0;
			// E/W
			if (token[0] == 'W') longitude *= -1;

		} else if (tokenctr == 6) {
			// fix quality
			if (token[0] == '\0' || token[0] == '0') {
				//serialPrint("parseGGA: invalid or missing fix quality...exiting.\r\n");
				return 0;
			}
		} else if (tokenctr == 7) {
			if (token[0] == '\0') return 0;
			// # sats
			if (atof(token) < 5) sat_check = 0;
		} else if (tokenctr == 9) {
			if (token[0] == '\0') return 0;
			// altitude
			//altitude = atof(token);
			// done parsing
			break;
		}

		// increment token pointers
		tokenctr++;
		token = strtok_r(NULL, ",", &tokenptr);
	}

	if (sat_check) {
		gpsBuff[0] = latitude;
		gpsBuff[1] = longitude;
		//gpsBuff[2] = altitude;
		return 2;
	} else {
		//serialPrint("parseGGA: too little sats...exiting.\r\n");
		return 1;
	}
}

/**	Parsing a GLL sentence
 * 	INPUT:
 * 		buff - pointer to string containing sentence, $ expected at index 0
 * 		buffSize - length of buffer
 * 		gpsBuff - pointer to float array where data will be stored
 * 	OUTPUT:
 * 		uint8_t status - 2 if valid line, 1 if valid line but not enough sats, 0 otherwise
 * */
static uint8_t parseGLL(char* buff, uint32_t buffSize, float* gpsBuff) {
	//serialPrint("parseGLL sentence\r\n");

	float latitude = 0, longitude = 0;
	int valid = 0;
	// using strtok_r
	int tokenctr = 0;
	char* tokenptr;
	char* token = strtok_r(buff, ",", &tokenptr);
	while (token != NULL) {
		if (tokenctr == 1) {
			if (token[0] == '\0') return 0;
			// latitude
			float rawLatitude = atof(token);
			latitude = (int)(rawLatitude / 100.0f);
			latitude += (rawLatitude - latitude * 100) / 60.0f;
		} else if (tokenctr == 2) {
			if (token[0] == '\0') return 0;
			// N/S
			if (token[0] == 'S') latitude *= -1;

		} else if (tokenctr == 3) {
			if (token[0] == '\0') return 0;
			// longitude
			float rawLongitude = atof(token);
			longitude = (int)(rawLongitude / 100);
			longitude += (rawLongitude - longitude * 100) / 60.0f;
		} else if (tokenctr == 4) {
			if (token[0] == '\0') return 0;
			// E/W
			if (token[0] == 'W') longitude *= -1;

		} else if (tokenctr == 6) {
			if (token[0] == '\0') return 0;
			// valid bit
			if (token[0] == 'A') valid = 1;
			// done parsing
			break;
		}

		// increment token pointers
		tokenctr++;
		token = strtok_r(NULL, ",", &tokenptr);
	}

	if (valid) {
		gpsBuff[0] = latitude;
		gpsBuff[1] = longitude;
		return 2;
	} else {
		//serialPrint("parseGLL: invalid sentence...exiting.\r\n");
		return 0;
	}
}

/**	Parsing a RMC sentence
 * 	INPUT:
 * 		buff - pointer to string containing sentence, $ expected at index 0
 * 		buffSize - length of buffer
 * 		gpsBuff - pointer to float array where data will be stored
 * 	OUTPUT:
 * 		uint8_t status - 2 if valid line, 1 if valid line but not enough sats, 0 otherwise
 * */
static uint8_t parseRMC(char* buff, uint32_t buffSize, float* gpsBuff) {
	//serialPrint("parseRMC sentence\r\n");

	float latitude = 0, longitude = 0;
	int valid = 0;
	// using strtok_r
	int tokenctr = 0;
	char* tokenptr;
	char* token = strtok_r(buff, ",", &tokenptr);
	while (token != NULL) {
		if (tokenctr == 2) {
			if (token[0] == '\0') return 0;
			// status
			if (token[0] == 'A') valid = 1;
		} else if (tokenctr == 3) {
			if (token[0] == '\0') return 0;
			// latitude
			float rawLatitude = atof(token);
			latitude = (int)(rawLatitude / 100.0f);
			latitude += (rawLatitude - latitude * 100) / 60.0f;
		} else if (tokenctr == 4) {
			if (token[0] == '\0') return 0;
			// N/S
			if (token[0] == 'S') latitude *= -1;

		} else if (tokenctr == 5) {
			if (token[0] == '\0') return 0;
			// longitude
			float rawLongitude = atof(token);
			longitude = (int)(rawLongitude / 100.0f);
			longitude += (rawLongitude - longitude * 100) / 60.0f;
		} else if (tokenctr == 6) {
			if (token[0] == '\0') return 0;
			// E/W
			if (token[0] == 'W') longitude *= -1;
			// done parsing
			break;
		}
		//  increment pointers
		tokenctr++;
		token = strtok_r(NULL, ",", &tokenptr);
	}

	if (valid) {
		gpsBuff[0] = latitude;
		gpsBuff[1] = longitude;
		return 2;
	} else {
		//serialPrint("parseRMC: invalid sentence...exiting.\r\n");
		return 0;
	}
}

/**	Reading a line of valid data output in blocking mode
 * 	INPUT:
 * 		gpsData - pointer to float array where lat, long, and alt data will be stored
 *  OUTPUT:
 *  	either 2 or 3 indicating number of data fields read, loop continuously reads until valid data
 * */
void neo8m_readData(float* gpsDataBuff) {
	uint8_t maxAttempts = 20;

	char buff[128];
	neo8m_readLine(buff, 128);
	maxAttempts--;
	while (neo8m_parseSentence(buff, 128, gpsDataBuff) != 2) {
		if (maxAttempts < 1) {
			serialPrint("neo8m_readData: exceeded max attempts..timeout.\r\n");
			return;
		}

		// keep trying new sentences
		HAL_Delay(10);
		neo8m_readLine(buff, 128);
		maxAttempts--;
	}
}


/****************************************************************************/





/**	Read a byte of data via interupt and store in buffer
 * 	INPUT:
 * 		byte - a byte of incoming data
 * */
void neo8m_readByte_IT(uint8_t byte) {
	// protecting against read/write collisions
	if ((gpsBufferWrite + 1) % GPS_BUFFER_LEN == gpsBufferRead) {
		serialPrint("neo8m_readByte_IT(), gpsBuffer overflow, exiting.\r\n");
		return;
	}

	gpsBuffer[gpsBufferWrite] = byte;
	gpsBufferWrite = (gpsBufferWrite + 1) % GPS_BUFFER_LEN;

	// if byte is newline character - '\n', then raise flag -> main loop reads flag and processes sentence
	if (byte == '\n') {
		__disable_irq();
		sentenceReadyFlag = 1;
		__enable_irq();
	}
}

/** Read a sentence from buffer and store in temp buffer, then parse sentence and update state data
 * */
void neo8m_processSentence_IT() {
	uint16_t sentenceBufferIDX = 0;
	while (gpsBuffer[gpsBufferRead] != '\n') {
		// read/write collision
		if (gpsBufferRead == gpsBufferWrite) {
			serialPrint("neo8m_processSentence_IT(): gpsBuffer read collision, exiting.\r\n");
			sentenceBufferIDX = 0;
			return;
		}

		sentenceBuffer[sentenceBufferIDX++] = gpsBuffer[gpsBufferRead];
		gpsBufferRead = (gpsBufferRead + 1) % GPS_BUFFER_LEN;

		if (sentenceBufferIDX >= GPS_TEMP_BUFFER_LEN) {
			sentenceBufferIDX = 0;
			serialPrint("neo8m_readSentence_IT(): sentenceBuffer overflow, exiting.\r\n");
			return;
		}
	}

	sentenceBuffer[sentenceBufferIDX++] = '\n';
	sentenceBuffer[sentenceBufferIDX] = '\0';
	sentenceBufferIDX = 0;
	float gpsDataBuff[3];
	neo8m_parseSentence(sentenceBuffer, GPS_TEMP_BUFFER_LEN, gpsDataBuff);
	// update gpsData - avoid race
	__disable_irq();
	gpsData[0] = gpsDataBuff[0];
	gpsData[1] = gpsDataBuff[1];
	gpsData[2] = gpsDataBuff[2];
	__enable_irq();
}

/**	Reading a line of data saved internally, updated in interrupt routine
 * 	INPUT:
 * 		external_gpsDataBuff - pointer to float array where lat, long, and alt data will be stored
 * */
void neo8m_readData_IT(float* external_gpsDataBuff) {
	// avoid race when we are reading gpsData while trying to update it
	__disable_irq();
	external_gpsDataBuff[0] = gpsData[0];
	external_gpsDataBuff[1] = gpsData[1];
	external_gpsDataBuff[2] = gpsData[2];
	__enable_irq();
}

/** Check if sentence flag ready
 * 	OUTPUT:
 * 		0 or 1
 * */
uint8_t neo8m_isSentenceReady_IT() {
	if (sentenceReadyFlag) {
		__disable_irq();
		sentenceReadyFlag = 0;
		__enable_irq();

		return 1;
	} else {
		return 0;
	}
}

