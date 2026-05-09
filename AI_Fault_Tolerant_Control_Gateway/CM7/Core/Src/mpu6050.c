#include "MPU6050.h"


/**********************************************************************************
 * @author    Mehmet Alperen BAKICI
 * @date      08.05.2026
 * @note      Control Gateway system initialization and peripheral management.
 *            Ensure I2C1 is initialized before calling Gateway_Init.
 * @retval    HAL_StatusTypeDef: HAL_OK (Success), HAL_ERROR (Failure)
 ***********************************************************************************/
HAL_StatusTypeDef MPU6050_Gateway_Init(I2C_HandleTypeDef *hi2c, MPU6050_t *pMpu)
{
    if (pMpu == NULL) return HAL_ERROR;

    Serial_Log_Message("[INFO] MPU6050 Local Init Started...\r\n");

    if (MPU6050_Init(hi2c) == HAL_OK)
    {
        pMpu->is_initialized = 1;
        Serial_Log_Message("[SUCCESS] MPU6050 Ready.\r\n");
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        return HAL_OK;
    }
    else
    {
        pMpu->is_initialized = 0;
        pMpu->error_cnt++;
        Serial_Log_Message("[ERROR] MPU6050 Fail!\r\n");
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        return HAL_ERROR;
    }
}

/**********************************************************************************
 * @author    Mehmet Alperen BAKICI
 * @date      03.05.2026
 * @note      Initialize and Configure MPU6050 / MPU6500 Sensor
 * @retval    HAL_StatusTypeDef: HAL_OK (Success), HAL_ERROR (Failure)
 ***********************************************************************************/
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t check;
    uint8_t data;
    HAL_StatusTypeDef status;

    /* 1. Verify Device Identity (WHO_AM_I register)
     * MPU6050 expected: 0x68 | MPU6500 expected: 0x70 */
    status = HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, REG_WHO_AM_I, 1, &check, 1, 100);

    if (status != HAL_OK) {
        return HAL_ERROR; /* Hardware-level I2C communication error */
    }

    /* Identity Check: Extended to support both MPU6050 and MPU6500 variants */
    if (check == 0x68 || check == 0x70) {

        /* 2. Power Management 1 (0x6B) - Wake up the device
         * Device starts in sleep mode (0x40) by default. Write 0 to wake all sensors. */
        data = 0x00;
        status = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, REG_PWR_MGMT_1, 1, &data, 1, 100);
        if (status != HAL_OK) return HAL_ERROR;

        /* Short delay to allow PLL and sensors to stabilize */
        HAL_Delay(10);

        /* 3. Configuration (0x1A) - Digital Low Pass Filter (DLPF)
         * Set filter to ~42Hz bandwidth to reduce high-frequency noise */
        data = 0x03;
        HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, 0x1A, 1, &data, 1, 100);

        /* 4. Accelerometer Configuration (0x1C) - Full Scale Range
         * 0x00 -> +/- 2g range (Sensitivity: 16384 LSB/g) */
        data = 0x00;
        status = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, REG_ACCEL_CONFIG, 1, &data, 1, 100);
        if (status != HAL_OK) return HAL_ERROR;

        /* 5. Gyroscope Configuration (0x1B) - Full Scale Range
         * 0x00 -> +/- 250 deg/s (Sensitivity: 131 LSB/deg/s) */
        data = 0x00;
        status = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, REG_GYRO_CONFIG, 1, &data, 1, 100);
        if (status != HAL_OK) return HAL_ERROR;

        return HAL_OK; /* Configuration successful */
    }

    /* Code reaches here if identity check fails */
    return HAL_ERROR;
}


/**********************************************************************************
 * @author    Mehmet Alperen BAKICI
 * @date      03.05.2026
 * @note      Read all sensor data (Accel, Temp, Gyro) in a single burst
 * @retval    HAL_StatusTypeDef: HAL_OK or HAL_ERROR
 ***********************************************************************************/
HAL_StatusTypeDef MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_t *DataStruct) {
    uint8_t Rec_Data[14]; /* Buffer for 6 Accel + 2 Temp + 6 Gyro bytes */

    /* Read 14 consecutive registers starting from REG_ACCEL_XOUT_H (0x3B)
     * Sequential burst read is highly efficient for I2C communication */
    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, REG_ACCEL_XOUT_H, 1, Rec_Data, 14, 100) == HAL_OK) {

        /* 1. Process Accelerometer Data (Scale: 16384 LSB/g) */
        DataStruct->Accel_X = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]) / 16384.0f;
        DataStruct->Accel_Y = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]) / 16384.0f;
        DataStruct->Accel_Z = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]) / 16384.0f;

        /* 2. Process Temperature Data (Formula from datasheet) */
        int16_t temp_raw = (int16_t)(Rec_Data[6] << 8 | Rec_Data[7]);
        DataStruct->Temperature = (temp_raw / 340.0f) + 36.53f;

        /* 3. Process Gyroscope Data (Scale: 131 LSB/deg/s) */
        DataStruct->Gyro_X = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]) / 131.0f;
        DataStruct->Gyro_Y = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]) / 131.0f;
        DataStruct->Gyro_Z = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]) / 131.0f;

        return HAL_OK;
    }
    return HAL_ERROR;
}
