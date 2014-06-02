/*
 August 2013

 Focused Flight32 Rev -

 Copyright (c) 2013 John Ihlein.  All rights reserved.

 Open Source STM32 Based Multicopter Controller Software

 Designed to run on the Naze32Pro Flight Control Board

 Includes code and/or ideas from:

 1)AeroQuad
 2)BaseFlight
 3)CH Robotics
 4)MultiWii
 5)Paparazzi UAV
 5)S.O.H. Madgwick
 6)UAVX

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

///////////////////////////////////////////////////////////////////////////////
#include "board.h"
#include "mw.h"

// Registers
#define MPU6000_SMPLRT_DIV	    	0x19
#define MPU6000_GYRO_CONFIG	    	0x1B
#define MPU6000_ACCEL_CONFIG  		0x1C
#define MPU6000_FIFO_EN		    	0x23
#define MPU6000_INT_PIN_CFG	    	0x37
#define MPU6000_INT_ENABLE	    	0x38
#define MPU6000_INT_STATUS	    	0x3A
#define MPU6000_ACCEL_XOUT_H 		0x3B
#define MPU6000_ACCEL_XOUT_L 		0x3C
#define MPU6000_ACCEL_YOUT_H 		0x3D
#define MPU6000_ACCEL_YOUT_L 		0x3E
#define MPU6000_ACCEL_ZOUT_H 		0x3F
#define MPU6000_ACCEL_ZOUT_L    	0x40
#define MPU6000_TEMP_OUT_H	    	0x41
#define MPU6000_TEMP_OUT_L	    	0x42
#define MPU6000_GYRO_XOUT_H	    	0x43
#define MPU6000_GYRO_XOUT_L	    	0x44
#define MPU6000_GYRO_YOUT_H	    	0x45
#define MPU6000_GYRO_YOUT_L	     	0x46
#define MPU6000_GYRO_ZOUT_H	    	0x47
#define MPU6000_GYRO_ZOUT_L	    	0x48
#define MPU6000_USER_CTRL	    	0x6A
#define MPU6000_SIGNAL_PATH_RESET   0x68
#define MPU6000_PWR_MGMT_1	    	0x6B
#define MPU6000_PWR_MGMT_2	    	0x6C
#define MPU6000_FIFO_COUNTH	    	0x72
#define MPU6000_FIFO_COUNTL	    	0x73
#define MPU6000_FIFO_R_W		   	0x74
#define MPU6000_WHOAMI		    	0x75

// Bits
#define BIT_SLEEP				    0x40
#define BIT_H_RESET				    0x80
#define BITS_CLKSEL				    0x07
#define MPU_CLK_SEL_PLLGYROX	    0x01
#define MPU_CLK_SEL_PLLGYROZ	    0x03
#define MPU_EXT_SYNC_GYROX		    0x02
#define BITS_FS_250DPS              0x00
#define BITS_FS_500DPS              0x08
#define BITS_FS_1000DPS             0x10
#define BITS_FS_2000DPS             0x18
#define BITS_FS_2G                  0x00
#define BITS_FS_4G                  0x08
#define BITS_FS_8G                  0x10
#define BITS_FS_16G                 0x18
#define BITS_FS_MASK                0x18
#define BITS_DLPF_CFG_42HZ          0x03
#define BITS_DLPF_CFG_20HZ          0x04
#define BITS_DLPF_CFG_10HZ          0x05
#define BITS_DLPF_CFG_5HZ           0x06
#define BITS_DLPF_CFG_2100HZ_NOLPF  0x07
#define BITS_DLPF_CFG_MASK          0x07
#define BIT_INT_ANYRD_2CLEAR        0x10
#define BIT_RAW_RDY_EN			    0x01
#define BIT_I2C_IF_DIS              0x10
#define BIT_INT_STATUS_DATA		    0x01
#define BIT_GYRO                    3
#define BIT_ACC                     2
#define BIT_TEMP                    1

static sensor_align_e gyroAlign = CW180_DEG;
static sensor_align_e accAlign = CW180_DEG;

// TODO read temperature
//   rawMPU6000Temperature[1] = spiTransfer(MPU6000_SPI, 0x00);
//   rawMPU6000Temperature[0] = spiTransfer(MPU6000_SPI, 0x00);

void mpu6000GyroInit(sensor_align_e align)
{
    if (align > 0){
        gyroAlign = align;
    }
}

void mpu6000AccInit(sensor_align_e align)
{
    if (align > 0){
        accAlign = align;
    }
//    if (mpuAccelHalf)
        acc_1G = 255 * 8;
//    else
//        acc_1G = 512 * 8;
}

bool mpu6000DetectSpi(sensor_t *acc, sensor_t *gyro, uint16_t lpf, uint8_t *scale)
{
    int16_t data[3];
    spiResetErrorCounter(MPU6000_SPI);

    setSPIdivisor(MPU6000_SPI, 128);  // 0.5625 MHz SPI Clock

    ///////////////////////////////////

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_PWR_MGMT_1);          // Device Reset
    spiTransfer(MPU6000_SPI, BIT_H_RESET);
    DISABLE_MPU6000;

    delay(150);

    spiTransfer(MPU6000_SPI, MPU6000_SIGNAL_PATH_RESET);          // Device Reset
    spiTransfer(MPU6000_SPI, BIT_GYRO | BIT_ACC | BIT_TEMP);

    delay(150);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_PWR_MGMT_1);          // Clock Source PPL with Z axis gyro reference
    spiTransfer(MPU6000_SPI, MPU_CLK_SEL_PLLGYROZ);
    DISABLE_MPU6000;

    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_USER_CTRL);           // Disable Primary I2C Interface
    spiTransfer(MPU6000_SPI, BIT_I2C_IF_DIS);
    DISABLE_MPU6000;

    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_PWR_MGMT_2);
    spiTransfer(MPU6000_SPI, 0x00);
    DISABLE_MPU6000;

    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_SMPLRT_DIV);          // Accel Sample Rate 1000 Hz, Gyro Sample Rate 8000 Hz
    spiTransfer(MPU6000_SPI, 0x00);
    DISABLE_MPU6000;

    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_CONFIG);              // Accel and Gyro DLPF Setting
    spiTransfer(MPU6000_SPI, BITS_DLPF_CFG_42HZ);
    DISABLE_MPU6000;

    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_ACCEL_CONFIG);        // Accel +/- 8 G Full Scale
    spiTransfer(MPU6000_SPI, BITS_FS_8G);
    DISABLE_MPU6000;

    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_GYRO_CONFIG);         // Gyro +/- 1000 DPS Full Scale
    spiTransfer(MPU6000_SPI, BITS_FS_2000DPS);
    DISABLE_MPU6000;

    mpu6000GyroRead(data);

    if ((((int8_t)data[1]) == -1 && ((int8_t)data[0]) == -1) || spiGetErrorCounter(HMC5983_SPI) != 0) {
        spiResetErrorCounter(MS5611_SPI);
        return false;
    }
    acc->init = mpu6000AccInit;
    acc->read = mpu6000AccRead;
    gyro->init = mpu6000GyroInit;
    gyro->read = mpu6000GyroRead;
    gyro->scale = (4.0f / 16.4f) * (M_PI / 180.0f) * 0.000001f;
    delay(100);
    return true;
}

bool mpu6000GyroRead(int16_t *gyroData)
{
    int16_t data[3];
    uint8_t buf[6];
    setSPIdivisor(MPU6000_SPI, 2);  // 18 MHz SPI clock

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_GYRO_XOUT_H | 0x80);

    buf[0] = spiTransfer(MPU6000_SPI, 0x00);
    buf[1] = spiTransfer(MPU6000_SPI, 0x00);
    buf[2] = spiTransfer(MPU6000_SPI, 0x00);
    buf[3] = spiTransfer(MPU6000_SPI, 0x00);
    buf[4] = spiTransfer(MPU6000_SPI, 0x00);
    buf[5] = spiTransfer(MPU6000_SPI, 0x00);

    DISABLE_MPU6000;

    data[ROLL] = (int16_t)((buf[0] << 8) | buf[1]) / 4;
    data[PITCH] = (int16_t)((buf[2] << 8) | buf[3]) / 4;
    data[YAW] = (int16_t)((buf[4] << 8) | buf[5]) / 4;

    alignSensors(data, gyroData, gyroAlign);
    return true;
}

bool mpu6000AccRead(int16_t *gyroData)
{
    int16_t data[3];
    uint8_t buf[6];
    setSPIdivisor(MPU6000_SPI, 2);  // 18 MHz SPI clock

    ENABLE_MPU6000;
    spiTransfer(MPU6000_SPI, MPU6000_ACCEL_XOUT_H | 0x80);

    buf[0] = spiTransfer(MPU6000_SPI, 0x00);
    buf[1] = spiTransfer(MPU6000_SPI, 0x00);
    buf[2] = spiTransfer(MPU6000_SPI, 0x00);
    buf[3] = spiTransfer(MPU6000_SPI, 0x00);
    buf[4] = spiTransfer(MPU6000_SPI, 0x00);
    buf[5] = spiTransfer(MPU6000_SPI, 0x00);

    DISABLE_MPU6000;

    data[ROLL] = (int16_t)((buf[0] << 8) | buf[1]) / 4;
    data[PITCH] = (int16_t)((buf[2] << 8) | buf[3]) / 4;
    data[YAW] = (int16_t)((buf[4] << 8) | buf[5]) / 4;

    alignSensors(data, gyroData, accAlign);
    return true;
}
