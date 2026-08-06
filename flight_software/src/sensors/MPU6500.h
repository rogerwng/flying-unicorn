#pragma once

#include "MPU6500_Registers.h"

#include "stm32f4xx_hal.h"

#include <stdfloat>

struct MPU6500_Data_t
{
    std::float32_t accelX;
    std::float32_t accelY;
    std::float32_t accelZ;
    std::float32_t gyroX;
    std::float32_t gyroY;
    std::float32_t gyroZ;
};

/**
 * @brief Integrated accelerometer and gyroscope over SPI
 */
class MPU6500
{
public:
    MPU6500() = default;
    ~MPU6500() = default;

    /**
     * @brief Initialize the MPU6500 IMU driver
     * @param hspi Handle of the SPI interface
     * @return True if successful, False otherwise
     */
    bool initialize(SPI_HandleTypeDef* hspi);

    /**
     * @brief Read data (blocking) from the MPU6500
     * @param dataOut Output buffer to store data
     * @return True if successful, False otherwise
     */
    bool readData(MPU6500_Data_t& dataOut);

private:
    SPI_HandleTypeDef* _hspi;

    /**
     * @brief Read an 8bit register from the device
     * @param address Register address to read from
     * @param data Buffer to store output
     * @return True if succesful, False otherwise
     */
    bool _readRegister(uint8_t address, uint8_t& data);

    /**
     * @brief Write an 8bit register to the device
     * @param address Register address to write to
     * @param data Byte to write
     * @return True if succesful, False otherwise
     */
    bool _writeRegister(uint8_t address, uint8_t data);
};

// ===========================
// Implementation
// ===========================

bool MPU6500::_readRegister(uint8_t address, uint8_t& data)
{
    (void)address;
    (void)data;
    return false;
}

bool MPU6500::_writeRegister(uint8_t address, uint8_t data)
{
    (void)address;
    (void)data;
    return false;
}

bool MPU6500::initialize(SPI_HandleTypeDef* hspi)
{
    bool ret = false;

    if (hspi != nullptr)
    {
        _hspi = hspi;
        ret = true;
    }

    return et;
}

bool MPU6500::readData(MPU6500_Data_t& dataOut)
{
    (void)dataOut;
    return false;
}