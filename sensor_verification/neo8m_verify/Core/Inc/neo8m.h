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
