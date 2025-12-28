/**
 * Thread safe Logging Utility that prints via USB-CDC
 */

#include <stdbool.h>

#define LOG_MAX_TAG_LEN 8
#define LOG_MAX_MSG_LEN 116
#define LOG_QUEUE_SIZE 16

// Total msg size for nominal logs 
#define LOG_TOTAL_MSG_SIZE (LOG_MAX_TAG_LEN + LOG_MAX_MSG_LEN + 17) // TAG + MSG + '[8 digits] [] \r\n' + null terminator
// Total msg size for direct logs
#define LOG_TOTAL_MSG_SIZE_DIRECT (LOG_MAX_TAG_LEN + LOG_MAX_MSG_LEN + 6) // TAG + MSG + '[] \r\n' + null terminator


typedef struct {
    const uint32_t timeTicks;
    const char* tag;
    const char msg[LOG_MAX_MSG_LEN];
} log_msg_t;

/**
 * @brief Initializes the logging system: USB_CDC, RTOS queue
 * @returns True on success, False otherwise
 */
bool Logger_Init();

/**
 * @brief Formats a message and pushes it to the log queue
 * @param tag Prefix TAG for the message
 * @param format Message format string
 * @param ... Variable input args to format string
 */
void LOG(const char* tag, const char* format, ...);

/**
 * @brief Formats a message and immediately uses CDC to print,
 *        intended to be used ONLY before LoggerTask starts
 * @param tag Prefix TAG for the message
 * @param format Message format string
 * @param ... Variable input args to format string
 */
void LOG_DIRECT(const char* tag, const char* format, ...);

/**
 * @brief Logger worker task, pops log queue and prints via CDC
 * @param argument No arguments expected
 */
void LoggerTask(void *argument);