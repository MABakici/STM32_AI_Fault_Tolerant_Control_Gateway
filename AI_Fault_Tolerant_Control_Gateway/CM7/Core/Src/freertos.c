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
#include <control_gateway_drv8833_driver.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include "math.h"
#include "mpu6050.h"
#include "global_variables_CM7.h"
#include "control_gateway_comm_handler.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

/* External handle for USART3 defined in main.c */
extern UART_HandleTypeDef huart3;
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
 * @author Mehmet Alperen BAKICI
 * @date   14.05.2026
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
 * @author Mehmet Alperen BAKICI
 * @date   14.05.2026
 * @details Processes IMU data for Impact and Tilt detection.
 */
void StartAnalysisTask(void *argument)
{
  /* Reduced filter count for faster response (20ms * 5 = 100ms debounce) */
  uint16_t tilt_filter_cnt = 0;

  for(;;)
  {
    if ( Global_t.mpu6050_veriler.is_initialized )
    {
      float ax = Global_t.mpu6050_veriler.Accel_X;
      float ay = Global_t.mpu6050_veriler.Accel_Y;
      float az = Global_t.mpu6050_veriler.Accel_Z;

      /* --- IMPACT DETECTION --- */
      float total_g = sqrtf((ax * ax) + (ay * ay) + (az * az));

      if (total_g > 1.5f || total_g < 0.5f)
      {
        Global_t.is_impact_detected = 1;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        /* WARNING: High osDelay here blocks the entire task!
           Reduced to 10ms for minimal task blocking */
        osDelay(10);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
      }
      else {
        Global_t.is_impact_detected = 0;
      }

      /* --- TILT DETECTION --- */
      /* az < 0.65f represents roughly 50 degrees of tilt */
      if (az < 0.65f)
      {
        /* Faster response: trigger after 5 consecutive samples (100ms total) */
        if (++tilt_filter_cnt > 5)
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
    /* Increased task frequency to 50Hz (Matches IMU sampling) */
    osDelay(20);
  }
}

/**
 * @brief  Task 3: System Heartbeat (Low Priority)
 * @author Mehmet Alperen BAKICI
 * @date   14.05.2026
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

/**
 * @brief  Task: Communication & UI Gateway (Normal Priority)
 * @author Mehmet Alperen BAKICI
 * @date   14.05.2026
 * @details Handles UART packet processing and orchestrates 20x4 LCD
 *          telemetry display including Uptime, IMU status, and Motor states.
 */
void StartCommTask(void *argument)
{
    char line_buf[21];
    uint32_t sec = 0, min = 0, hour = 0;
    Comm_Handle_t hGatewayComm = {
        .pRxBuffer     = Global_t.uart_gateway.rx_buffer,
        .pErrorCounter = &Global_t.uart_gateway.uart_error_cnt
    };

    HAL_UART_Receive_DMA(&huart3, Global_t.uart_gateway.rx_buffer, FRAME_LENGTH);
    LCD_Clear();
    osDelay(100);

    for(;;)
    {
        uint32_t current_tick = HAL_GetTick();

        /* --- ROW 0: UPTIME --- */
        sec = (current_tick / 1000) % 60;
        min = (current_tick / 60000) % 60;
        hour = (current_tick / 3600000);
        sprintf(line_buf, "UPTIME: %02lu:%02lu:%02lu    ", hour, min, sec);
        LCD_Put_Cursor(0, 0);
        LCD_Send_String(line_buf);

        /* --- ROW 1: UART MONITORING & VISUAL LOCK --- */
        /* Eğer paket geldiyse işle, bayrakları Comm_Process_Packet kaldıracak */
        if (Global_t.uart_gateway.packet_ready == 1) {
            Comm_Process_Packet(&hGatewayComm);
            Global_t.uart_gateway.packet_ready = 0;
        }

        LCD_Put_Cursor(1, 0);
        /* GÖRSEL KİLİT KONTROLÜ: display_timeout_flag üzerinden çalışır */
        if (Global_t.uart_gateway.display_timeout_flag == 1)
        {
            /* 3 saniye dolmadıysa mesajı koru */
            if (current_tick - Global_t.uart_gateway.last_packet_tick < 3000)
            {
                LCD_Send_String("UART: PKT RECEIVED  ");
            }
            else
            {
                /* Süre doldu, kilidi kaldır ve normale dön */
                Global_t.uart_gateway.display_timeout_flag = 0;
                LCD_Send_String("UART: LISTENING     ");
            }
        }
        else
        {
            /* Boşta kalma durumu */
            LCD_Send_String("UART: LISTENING     ");
        }

        /* --- ROW 2: IMU STATUS --- */
        LCD_Put_Cursor(2, 0);
        if (Global_t.is_tilt_detected)       LCD_Send_String("IMU: TILT DETECTED  ");
        else if (Global_t.is_impact_detected) LCD_Send_String("IMU: IMPACT DETECTED");
        else                                LCD_Send_String("IMU: SYSTEM STABLE  ");

        /* --- ROW 3: MOTOR STATUS --- */
        LCD_Put_Cursor(3, 0);
        if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted) {
            LCD_Send_String("MOT: HALTED         ");
        } else {
            /* Dinamik hız gösterimi */
            sprintf(line_buf, "MOT: ACTIVE (%3d%%) ", Global_t.dc_motor_control.target_speed);
            LCD_Send_String(line_buf);
        }

        osDelay(250); // 4Hz refresh rate
    }
}


/**
 * @brief  Task: Motor Actuation & Safety Control (Normal Priority)
 * @author Mehmet Alperen BAKICI
 * @date   14.05.2026
 * @details Implements a safety-first motor control logic. Priorities:
 *          1. Physical Safety (Tilt)
 *          2. Logical Safety (Software Halt)
 *          3. Operational Speed (Asynchronous PWM updates)
 */
void StartMotorCntTask(void *argument)
{
    /* Initialize actuator defaults to ensure predictable startup state */
    Global_t.dc_motor_control.target_speed = 50;
    Global_t.dc_motor_control.is_system_halted = 0;

    for(;;)
    {
        /**
         * @section Safety Priority Logic
         * Immediate shutdown if physical tilt is detected OR
         * software-triggered emergency stop is active.
         */
        if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
        {
            /* Force immediate actuator cessation */
            DRV8833_Set_Motor(MOTOR_A, MOTOR_STOP, 0);
        }
        else
        {
            /**
             * @section Operational Mode
             * Resume nominal operation using asynchronously updated PWM duty cycle.
             */
            DRV8833_Set_Motor(MOTOR_A, MOTOR_FORWARD, Global_t.dc_motor_control.target_speed);
        }

        /* Enforce task frequency (20Hz) for optimized power/performance ratio */
        osDelay(50);
    }
}


/* USER CODE END Application */

