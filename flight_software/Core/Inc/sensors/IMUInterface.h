/**
 * Defines interface for an abstract IMU device
 */
#include "SystemInitializer.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief A single sample of raw IMU data
 */
typedef struct {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} imu_sample_t;

/**
 * @brief A repository for a sample of IMU data at a specific point in time
 * 
 * @param seq_n A monotonic sequence number increasing with every sample collected
 * @param timestamp The timestamp in microseconds since system boot 
 */
typedef struct {
    imu_sample_t data;
    uint32_t seq_n;
    uint64_t timestamp_us;
} imu_repo_t;

/**
 * @brief Initialize the IMU device
 * 
 * @param rate Rate of IMU collection in Hz
 */
bool Imu_Init(SystemHardwareHandles_t hardwareHandles, uint16_t rate);

/**
 * @brief Get a sample of IMU data
 * 
 * @param imuBuff Pointer to an imu_sample_t to fill with data
 */
void Imu_GetSample(imu_sample_t* imuBuff);