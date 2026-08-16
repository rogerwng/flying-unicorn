#pragma once

#include "MPU6500_Registers.h"

#include "stm32f4xx_hal.h"

#include <stdfloat>
#include <span>

struct MPU6500_Data_t
{
    std::float32_t accelX;
    std::float32_t accelY;
    std::float32_t accelZ;
    std::float32_t gyroX;
    std::float32_t gyroY;
    std::float32_t gyroZ;
};

struct MPU6500_Config_t
{
    SPI_HandleTypeDef* hspi;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    uint32_t spi_timeout_ms;
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
     * @param config Configuration for the MPU6500
     * @return True if successful, False otherwise
     */
    bool initialize(MPU6500_Config_t config);

    /**
     * @brief Read data (blocking) from the MPU6500
     * @param dataOut Output buffer to store data
     * @return True if successful, False otherwise
     */
    bool readData(MPU6500_Data_t& dataOut);

private:
    MPU6500_Config_t _config;

    /**
     * @brief Verify configuration is valid
     * @param config Configuration for the MPU6500
     * @return True if succesful, False otherwise
     */
    bool _verify_config(MPU6500_Config_t config);

    /**
     * @brief Read data starting from register
     * @param address Register address to start reading from
     * @param data Buffer to store output
     * @return True if succesful, False otherwise
     */
    bool _readRegisters(uint8_t address, uint8_t& data);

    /**
     * @brief Write data starting from register
     * @param address Register address to start writing at
     * @param data Bytes to write
     * @return True if succesful, False otherwise
     */
    bool _writeRegisters(uint8_t address, std::span<const uint8_t> data);
};

// ===========================
// Implementation
// ===========================

bool MPU6500::initialize(MPU6500_Config_t config)
{
    bool ret = _verify_config(config);

    if (ret)
    {
        _config = config;
    }

    return ret;
}

bool MPU6500::readData(MPU6500_Data_t& dataOut)
{
    (void)dataOut;
    return false;
}

bool MPU6500::_verify_config(MPU6500_Config_t config)
{
    bool ret = false;
    
    if (config.hspi != nullptr && config.cs_port != nullptr)
    {
        ret = true;
    }

    return ret;
}

bool MPU6500::_readRegister(uint8_t address, std::span<uint8_t> data)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t readAddress = address & MPU6500_REG::READ_MASK;

    if (_config.hspi != nullptr && _config.cs_port != nullptr)
    {
        HAL_GPIO_WritePin(_config.cs_port, _config.cs_pin, GPIO_PIN_RESET);
        status = HAL_SPI_Transmit(_config.hspi, &readAddress, 1, _config.spi_timeout_ms);
        status = (status == HAL_OK) ? HAL_SPI_Receive(_config.hspi, data.data(), data.size(), _config.spi_timeout_ms) : status;
        HAL_GPIO_WritePin(_config.cs_port, _config.cs_pin, GPIO_PIN_SET);
    }

    return status == HAL_OK;
}

bool MPU6500::_writeRegisters(uint8_t address, std::span<const uint8_t> data)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t writeAddress = address | MPU6500_REG::WRITE_MASK;

    if (_config.hspi != nullptr && _config.cs_port != nullptr)
    {
        HAL_GPIO_WritePin(_config.cs_port, _config.cs_pin, GPIO_PIN_RESET);
        status = HAL_SPI_Transmit(_config.hspi, &writeAddress, 1, _config.spi_timeout_ms);
        status = (status == HAL_OK) ? HAL_SPI_Transmit(_config.hspi, data.data(), data.size(), _config.spi_timeout_ms) : status;
        HAL_GPIO_WritePin(_config.cs_port, _config.cs_pin, GPIO_PIN_SET);
    }

    return status == HAL_OK;
}