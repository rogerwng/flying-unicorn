/**
 * Implements the IMUInterface for the MPU6500 over SPI
 */
#include "stm32f4xx_hal.h"

#include "IMUInterface.h"

#define WHO_AM_I_VALUE      (0x70)
#define REG_WHO_AM_I        (0x75)
#define REG_CONFIG          (0x1A) // FIFO mode, DLPF config
#define REG_GYRO_CONFIG     (0x1B) // full scale select, DLPF
#define REG_ACCEL_CONFIG    (0x1C) // full scale select
#define REG_ACCEL_CONFIG2   (0x1D) // DLPF
#define REG_DATA_BASE       (0x3B) // start of data reg
#define DATA_LEN_BYTES      (14U)  // 14 bytes 16bit MSB, 6 accel + 2 temp + 6 gyro

static SPI_HandleTypeDef* hspi;
static uint16_t imuRate;

bool Imu_Init(SystemHardwareHandles_t hardwareHandles, uint16_t rate)
{
    hspi = hardwareHandles.p_hspi2;
    imuRate = rate;

    // Verify imu connection
}

void Imu_GetSample(imu_sample_t* imuBuff);