#include "main.h"

#include "BoardConfig.h"
#include "Logger.h"

#include "MPU6500.h"

static const char TAG[] = "MAIN";

// App_Init is called before the freeRTOS scheduler runs - initialize all freeRTOS tasks/objects here
extern "C" void App_Init(void)
{
    // Initialize the logger
    if (!Logger::getInstance().initialize(LOGGER_HUART))
    {
        LOG_DIRECT(TAG, "Failed to initialize logger");
    }
}

// App_Run is called inside the default freeRTOS task
extern "C" void App_Run(void)
{
    while(1)
    {
        LOG(TAG, "Hello World!");
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        osDelay(1000);
    }
}