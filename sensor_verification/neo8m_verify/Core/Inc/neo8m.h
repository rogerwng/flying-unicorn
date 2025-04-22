/**
  ******************************************************************************
  * @file           : neo8m.h
  * @brief          : Header for neo8m.c file.
  *                   This file contains the defines of the NEO-8M interface.
  ******************************************************************************
  */

#include "main.h"

/** Initialize NEO8M via UART */
void neo8m_init(UART_HandleTypeDef* huart);

/** Read a line of NEO8M data in blocking mode */
void neo8m_readLine(char* buff, uint32_t buffSize);


/**	Parsing a GGA sentence */
uint8_t neo8m_parseSentence(char* buff, uint32_t buffSize, float* gpsBuff);

/**	Reading a line of valid dataoutput in blocking mode */
void neo8m_readData(float* gpsDataBuff);

/**	INTERUPT INTERFACE */

/** Read byte of data through interupt */
void neo8m_readByte_IT(uint8_t byte);

/** Once full sentence received in interupt handler (detects '\n'), parse sentence */
void neo8m_processSentence_IT();

/** Reading GPS data that is updated through interupt */
void neo8m_readData_IT(float* external_gpsDataBuff);

/** Check if sentence flag ready */
uint8_t neo8m_isSentenceReady_IT();
