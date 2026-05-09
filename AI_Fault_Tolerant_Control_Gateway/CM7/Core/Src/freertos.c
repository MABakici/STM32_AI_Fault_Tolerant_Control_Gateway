/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include "math.h"
#include "mpu6050.h"
#include "global_variables_CM7.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern osMutexId_t I2C2_MutexHandle;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief  Task 1: Sensor Data Acquisition (High Priority)
 * @details Reads raw data from MPU6050 via I2C with Mutex protection.
 */
void StartIMUReadTask(void *argument)
{
  for(;;)
  {
    if ( Global_t.mpu6050_veriler.is_initialized )
    {
      /* Mutex acquisition for I2C Bus Resource Protection */
      if (osMutexAcquire(I2C2_MutexHandle, osWaitForever) == osOK)
      {
        if (MPU6050_Read_All(&hi2c2, &Global_t.mpu6050_veriler) != HAL_OK)
        {
          Global_t.i2c_error_cnt++;
        }
        else
        {
          Global_t.i2c_error_cnt = 0;
        }
        /* Release I2C resource for other possible users */
        osMutexRelease(I2C2_MutexHandle);
      }
    }
    /* 50Hz sampling rate */
    osDelay(20);
  }
}

/**
 * @brief  Task 2: Data Analysis & LED Management (Normal Priority)
 * @details Processes IMU data for Impact and Tilt detection.
 */
void StartAnalysisTask(void *argument)
{
  uint16_t tilt_filter_cnt = 0;
  for(;;)
  {
    if ( Global_t.mpu6050_veriler.is_initialized )
    {
      float ax = Global_t.mpu6050_veriler.Accel_X;
      float ay = Global_t.mpu6050_veriler.Accel_Y;
      float az = Global_t.mpu6050_veriler.Accel_Z;

      /* --- IMPACT DETECTION (Red LED - PB14) --- */
      /* Calculate magnitude of the acceleration vector (G-Force) */
      float total_g = sqrtf((ax * ax) + (ay * ay) + (az * az));

      if (total_g > 1.5f || total_g < 0.5f)
      {
        Global_t.is_impact_detected = 1;
        /* Trigger momentary flash for impact alert */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        osDelay(40);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
      }
      else {
        Global_t.is_impact_detected = 0;
      }

      /* --- TILT DETECTION (Green LED - PB0) --- */
      /* 200ms debouncing filter to prevent false alarms during vibration */
      if (az < 0.65f)
      {
        if (++tilt_filter_cnt > 10)
        {
          Global_t.is_tilt_detected = 1;
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        }
      }
      else
      {
        tilt_filter_cnt = 0;
        Global_t.is_tilt_detected = 0;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
      }
    }
    osDelay(50);
  }
}

/**
 * @brief  Task 3: System Heartbeat (Low Priority)
 * @details Toggles Yellow LED to indicate scheduler and system health.
 */
void StartHeartbeatTask(void *argument)
{
  for(;;)
  {
    if ( Global_t.mpu6050_veriler.is_initialized )
    {
      /* 1Hz toggling frequency (500ms ON / 500ms OFF) */
      HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
    }
    osDelay(500);
  }
}


/* USER CODE END Application */

