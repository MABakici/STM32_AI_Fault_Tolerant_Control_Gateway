#ifndef MPU6050_H_
#define MPU6050_H_

#include "stm32h7xx_hal.h"
#include "control_gateway_comm.h"

/* MPU6050 I2C Slave Address (Default 0x68 shifted for HAL) */
#define MPU6050_ADDR (0x68 << 1)

/* --- MPU6050 Register Map --- */
#define REG_WHO_AM_I     0x75
#define REG_PWR_MGMT_1   0x6B
#define REG_ACCEL_XOUT_H 0x3B
#define REG_GYRO_XOUT_H  0x43  /* Starting register for Gyroscope data */
#define REG_TEMP_OUT_H   0x41  /* Starting register for Temperature data */
#define REG_GYRO_CONFIG  0x1B
#define REG_ACCEL_CONFIG 0x1C
#define REG_CONFIG       0x1A

/**
 * @brief  MPU6050 Data Structure
 * @note   Stores processed floating point values for all sensors
 */
typedef struct {
    /* Processed Accelerometer data in 'g' units */
    float Accel_X;
    float Accel_Y;
    float Accel_Z;

    /* Processed Gyroscope data in 'degrees per second' (dps) */
    float Gyro_X;
    float Gyro_Y;
    float Gyro_Z;

    /* Processed Temperature data in Celsius */
    float Temperature;

    /* Calculated orientation data */
    float Pitch;
    float Roll;

    /* --- Sensor Specific Status --- */
    uint8_t  is_initialized;
    uint32_t error_cnt;
} MPU6050_t;

/* --- Function Prototypes --- */

/**
 * @brief  Initializes the MPU6050 sensor with predefined configurations.
 * @param  hi2c: Pointer to I2C Handle
 * @retval HAL Status
 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief  Reads Accelerometer, Gyroscope, and Temperature data in one burst.
 * @param  hi2c: Pointer to I2C Handle
 * @param  DataStruct: Pointer to the structure where data will be stored
 * @retval HAL Status
 */
HAL_StatusTypeDef MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_t *DataStruct);

HAL_StatusTypeDef MPU6050_Gateway_Init(I2C_HandleTypeDef *hi2c, MPU6050_t *pMpu);

#endif /* MPU6050_H_ */
