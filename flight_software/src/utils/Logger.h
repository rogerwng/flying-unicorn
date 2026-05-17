#pragma once

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <string>

// ===========================
// Logging utilities
// ===========================

/**
 * @brief Formats a message and pushes it to the log queue
 * @param tag Prefix TAG for the message
 * @param format Message format string
 * @param ... Variable input args to format string
 */
void LOG(const char* tag, const char* format, ...);

/**
 * @brief Formats a message and immediately logs it,
 *        intended to be used ONLY before LoggerTask starts
 * @param tag Prefix TAG for the message
 * @param format Message format string
 * @param ... Variable input args to format string
 */
void LOG_DIRECT(const char* tag, const char* format, ...);

// ===========================
// Logger
// ===========================

inline constexpr uint8_t LOG_MAX_TAG_LEN = 8;
inline constexpr uint8_t LOG_MAX_MSG_LEN = 116;
inline constexpr uint8_t LOG_QUEUE_SIZE = 16;

// Total msg size for nominal logs = TAG + MSG + '[8 digits] [] \r\n' + null terminator
inline constexpr uint8_t LOG_TOTAL_MSG_SIZE = (LOG_MAX_TAG_LEN + LOG_MAX_MSG_LEN + 17);
// Total msg size for direct logs = TAG + MSG + '[] \r\n' + null terminator
inline constexpr uint8_t LOG_TOTAL_MSG_SIZE_DIRECT = (LOG_MAX_TAG_LEN + LOG_MAX_MSG_LEN + 6);

struct LogMsg_t
{
    uint32_t timeTicks;
    const char* tag;
    char msg[LOG_MAX_MSG_LEN];
};

/**
 * @brief Singleton RTOS Logger with background log task
 */
class Logger 
{
public:
    // Enforce singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    static Logger& getInstance(void) 
    {
        static Logger instance;
        return instance;
    }

    static void loggerTaskEntry(void* arguments)
    {
        (void)arguments; // unused arg
        Logger::getInstance().loggerTask();
    }

    /**
     * @brief Enqueues log
     * @param msg Pointer to the log message
     */
    void enqueueLog(LogMsg_t* msg)
    {
        osMessageQueuePut(queue_, msg, 0, 0);
    }

    /**
     * @brief Check if logger task running
     */
    bool isLoggerRunning(void)
    {
        return isRunning_;
    }

    /**
     * @brief Initializes the logging system
     * @param huart Pointer to the logging UART handle
     * @returns True on success, False otherwise
     */
    bool initialize(UART_HandleTypeDef* huart);

    /**
     * @brief Logs a single message via blocking UART
     * @param buff Pointer to the message buffer
     * @param len Length of message
     */
    void log(uint8_t* buff, int len);

private:
    Logger() = default;
    ~Logger() = default;

    UART_HandleTypeDef* huart_;

    // RTOS Handles
    volatile bool isRunning_;
    osMessageQueueId_t queue_;
    osThreadId_t loggerTaskHandle_;

    /**
     * @brief Logger worker task, pops log queue and prints via blocking UART
     * @param argument No arguments expected
     */
    void loggerTask(void);
};

// ===========================
// Implementation
// ===========================

bool Logger::initialize(UART_HandleTypeDef* huart) 
{
    bool status = false;
    huart_ = huart;

    // Initialize message queue
    queue_ = osMessageQueueNew(LOG_QUEUE_SIZE, sizeof(LogMsg_t), NULL);
    if (queue_ != NULL)
    {
        status = true;
    }

    // Register logger task
    if (status)
    {
        osThreadAttr_t logTaskAttr{};
        logTaskAttr.name = "Logger";
        logTaskAttr.stack_size = 3072;
        logTaskAttr.priority = osPriorityLow;

        loggerTaskHandle_ = osThreadNew(Logger::loggerTaskEntry, NULL, &logTaskAttr);
    }

    return status;
}

void Logger::log(uint8_t* buff, int len) 
{
    if (huart_)
    {
        HAL_UART_Transmit(huart_, buff, len, HAL_MAX_DELAY);
    }
}

void Logger::loggerTask(void)
{
    isRunning_ = true;

    uint32_t tickFreq = osKernelGetTickFreq();
    uint32_t timeoutTicks = 500 * tickFreq / 1000U; // 500ms in ticks timeout

    LogMsg_t msgBuff{};
    char msg[LOG_TOTAL_MSG_SIZE] = {};
    while (isRunning_) 
    {
        // Pop off queue
        if (osMessageQueueGet(queue_, &msgBuff, NULL, timeoutTicks) != osOK) 
        {
            // timed out
            continue;
        }

        // Get time - truncated to 8 digits (rolls over in 24h)
        uint32_t timeMs = (msgBuff.timeTicks * 1000U) / tickFreq;
        timeMs %= 100000000;

        // Format string
        snprintf(msg, LOG_TOTAL_MSG_SIZE, "[%lu] [%s] %s\r\n", timeMs, msgBuff.tag, msgBuff.msg);

        log((uint8_t*)msg, strlen(msg));
    }
}

// ===========================
// Logging utilities
// ===========================

void LOG(const char* tag, const char* format, ...)
{
    // New message
    LogMsg_t msg;
    msg.timeTicks = osKernelGetTickCount();
    msg.tag = tag;

    // Format and fill  message
    va_list args;
    va_start(args, format);
    vsnprintf(msg.msg, LOG_MAX_MSG_LEN, format, args);
    va_end(args);

    // Push to queue
    Logger::getInstance().enqueueLog(&msg);
}

void LOG_DIRECT(const char* tag, const char* format, ...)
{
    // This function should only be used before the logger task is ready
    if (Logger::getInstance().isLoggerRunning())
    {
        return;
    }

    char msg[LOG_TOTAL_MSG_SIZE_DIRECT] = {}; 

    // Add tag first
    int offset = snprintf(msg, LOG_TOTAL_MSG_SIZE_DIRECT, "[%s] ", tag);
    if (offset < 0 || offset >= LOG_TOTAL_MSG_SIZE_DIRECT)
    {
        offset = 0; // snprintf fail
    }
    
    // Format msg
    va_list args;
    va_start(args, format);
    int msgLen = vsnprintf(msg + offset, LOG_TOTAL_MSG_SIZE_DIRECT - offset - 3, format, args);
    va_end(args);
    
    if (msgLen < 0)
    {
        msgLen = 0; // vsnprintf fail
    }
    
    int totalLen = offset + msgLen;
    msg[totalLen++] = '\r';
    msg[totalLen++] = '\n';
    msg[totalLen] = '\0';

    Logger::getInstance().log((uint8_t*)msg, totalLen);
}