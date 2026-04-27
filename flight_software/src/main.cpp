#include "main.h"

extern "C" void App_Init(void)
{

}

extern "C" void App_Run(void)
{
    while(1)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        HAL_Delay(500);
    }
}