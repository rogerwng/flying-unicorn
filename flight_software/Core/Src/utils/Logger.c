/**
 * Thread safe Logging Utility that prints via USB-CDC
 */

#include "Logger.h"

#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "cmsis_os2.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Logger tag
static const char TAG[] = "LOGGER";

// RTOS handles and flags
static volatile bool loggerTaskRunning;
static osMessageQueueId_t queue;

bool Logger_Init() {
    loggerTaskRunning = false;

    // Initialize the USB device
    MX_USB_DEVICE_Init();
    HAL_Delay(3500); // 3.5sec wait for PC to connect
    LOG_DIRECT(TAG, "Initialized the USB device");

    // Initialize the message queue
    queue = osMessageQueueNew(LOG_QUEUE_SIZE, sizeof(log_msg_t), NULL);
    if (queue == NULL) {
        LOG_DIRECT(TAG, "Fatal: Error creating log message queue");
        return false;
    }

    return true;
}

void LOG(const char* tag, const char* format, ...) {
    // New message
    log_msg_t msg;
    msg.timeTicks = osKernelGetTickCount();
    msg.tag = tag;

    // Format and fill  message
    va_list args;
    va_start(args, format);
    vsnprintf(msg.msg, LOG_MAX_MSG_LEN, format, args);
    va_end(args);

    // Push to queue
    osMessageQueuePut(queue, &msg, 0, 0);
}

void LOG_DIRECT(const char* tag, const char* format, ...) {
    // This function should only be used before the logger task is ready
    if (loggerTaskRunning)
        return;

    char msg[LOG_TOTAL_MSG_SIZE_DIRECT]; 

    // Add tag first
    int offset = snprintf(msg, LOG_TOTAL_MSG_SIZE_DIRECT, "[%s] ", tag);
    if (offset < 0 || offset >= LOG_TOTAL_MSG_SIZE_DIRECT)
        offset = 0; // snprintf fail
    
    // Format msg
    va_list args;
    va_start(args, format);
    int msgLen = vsnprintf(msg + offset, LOG_TOTAL_MSG_SIZE_DIRECT - offset - 3, format, args);
    va_end(args);
    
    if (msgLen < 0)
        msgLen = 0; // vsnprintf fail
    
    int totalLen = offset + msgLen;
    msg[totalLen++] = '\r';
    msg[totalLen++] = '\n';
    msg[totalLen] = '\0';

    CDC_Transmit_FS((uint8_t*)msg, totalLen);
}

void LoggerTask(void *argument) {
    loggerTaskRunning = true;

    // Set up timing constants
    uint32_t tickFreq = osKernelGetTickFreq();
    uint32_t timeoutTicks = 500 * tickFreq / 1000U; // 500ms in ticks timeout

    LOG(TAG, "Started Logger queue TX loop");

    log_msg_t msgBuff;
    char msg[LOG_TOTAL_MSG_SIZE];
    while (loggerTaskRunning) {
        // Pop off queue
        if (osMessageQueueGet(queue, &msgBuff, NULL, timeoutTicks) != osOK) {
            // timed out
            continue;
        }

        // Get time - truncated to 8 digits (rolls over in 24h)
        uint32_t timeMs = (msgBuff.timeTicks * 1000U) / tickFreq;
        timeMs %= 100000000;

        // Format string
        snprintf(msg, LOG_TOTAL_MSG_SIZE, "[%lu] [%s] %s\r\n", timeMs, msgBuff.tag, msgBuff.msg);

        // CDC Transmit
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    }
}