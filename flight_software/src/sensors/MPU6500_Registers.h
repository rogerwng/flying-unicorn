#pragma once

#include <cstdint>

namespace MPU6500_REG
{

// ===========================
// Register map
// ===========================

namespace CONFIG
{
    inline constexpr uint8_t ADDRESS = 0x1A;
    inline constexpr uint8_t RESET = 0x00;

    inline constexpr uint8_t FIFO_MODE = (1 << 6);
    inline constexpr uint8_t EXT_SYNC_SET_2 = (1 << 5);
    inline constexpr uint8_t EXT_SYNC_SET_1 = (1 << 4);
    inline constexpr uint8_t EXT_SYNC_SET_0 = (1 << 3);
    inline constexpr uint8_t DLPF_CFG_2 = (1 << 2);     // DLPF valid when FCHOICE == 11b or FCHOICE_B == 00b
    inline constexpr uint8_t DLPF_CFG_1 = (1 << 1);
    inline constexpr uint8_t DLPF_CFG_0 = (1 << 0);
};

namespace GYRO_CONFIG
{
    inline constexpr uint8_t ADDRESS = 0x1B;
    inline constexpr uint8_t RESET = 0x00;

    inline constexpr uint8_t XG_ST = (1 << 7);          // X Gyro self test
    inline constexpr uint8_t YG_ST = (1 << 6);          // Y Gyro self test
    inline constexpr uint8_t ZG_ST = (1 << 5);          // Z Gyro self test
    inline constexpr uint8_t GYRO_FS_SEL_1 = (1 << 4);  // Gyro Full Scale select [dps]: 00b = 250, 01b = 500, 10b = 1000, 11b = 2000
    inline constexpr uint8_t GYRO_FS_SEL_0 = (1 << 3);
    inline constexpr uint8_t FCHOICE_B_1 = (1 << 1);    // Bypass DLPF, see table page 14
    inline constexpr uint8_t FCHOICE_B_0 = (1 << 0);
};

namespace ACCEL_CONFIG
{
    inline constexpr uint8_t ADDRESS = 0x1C;
    inline constexpr uint8_t RESET = 0x00;

    inline constexpr uint8_t XA_ST = (1 << 7);          // X Accel self test
    inline constexpr uint8_t YA_ST = (1 << 6);          // Y Accel self test
    inline constexpr uint8_t ZA_ST = (1 << 5);          // Z Accel self test
    inline constexpr uint8_t ACCEL_FS_SEL_1 = (1 << 4); // Accel Full Scale select [g]: 00b = 2, 01b = 4, 10b = 8, 11b = 16
    inline constexpr uint8_t ACCEL_FS_SEL_0 = (1 << 3);
};

namespace ACCEL_CONFIG_2
{
    inline constexpr uint8_t ADDRESS = 0x1D;
    inline constexpr uint8_t RESET = 0x00;

    inline constexpr uint8_t ACCEL_FCHOICE_B = (1 << 3);    // Bypass DLPF, see page 15
    inline constexpr uint8_t A_DLPF_CFG_2 = (1 << 2);       // Accel low pass filter, see page 15
    inline constexpr uint8_t A_DLPF_CFG_1 = (1 << 1);
    inline constexpr uint8_t A_DLPF_CFG_0 = (1 << 0);
};

namespace ACCEL_XOUT_H
{
    inline constexpr uint8_t ADDRESS = 0x3B;
};

namespace ACCEL_XOUT_L
{
    inline constexpr uint8_t ADDRESS = 0x3C;
};

namespace ACCEL_YOUT_H
{
    inline constexpr uint8_t ADDRESS = 0x3D;
};

namespace ACCEL_YOUT_L
{
    inline constexpr uint8_t ADDRESS = 0x3E;
};

namespace ACCEL_ZOUT_H
{
    inline constexpr uint8_t ADDRESS = 0x3F;
};

namespace ACCEL_ZOUT_L
{
    inline constexpr uint8_t ADDRESS = 0x40;
};

namespace TEMP_OUT_H
{
    inline constexpr uint8_t ADDRESS = 0x41;
};

namespace TEMP_OUT_L
{
    inline constexpr uint8_t ADDRESS = 0x42;
};

namespace GYRO_XOUT_H
{
    inline constexpr uint8_t ADDRESS = 0x43;
};

namespace GYRO_XOUT_L
{
    inline constexpr uint8_t ADDRESS = 0x44;
};

namespace GYRO_YOUT_H
{
    inline constexpr uint8_t ADDRESS = 0x45;
};

namespace GYRO_YOUT_L
{
    inline constexpr uint8_t ADDRESS = 0x46;
};

namespace GYRO_ZOUT_H
{
    inline constexpr uint8_t ADDRESS = 0x47;
};

namespace GYRO_ZOUT_L
{
    inline constexpr uint8_t ADDRESS = 0x48;
};

namespace WHO_AM_I
{
    inline constexpr uint8_t ADDRESS = 0x75;
    inline constexpr uint8_t RESET = 0x70;
};

}   // namespace MPU6500_REG
