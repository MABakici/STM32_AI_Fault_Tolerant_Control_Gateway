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

/**
 * @brief  Task: Communication Gateway (Normal Priority)
 * @author Mehmet Alperen BAKICI
 * @date   11.05.2026
 * @note   Implements a non-blocking UI state machine for uptime and packet telemetry.
 */
void StartCommTask(void *argument)
{
  /* USER CODE BEGIN StartCommunicationTask */

  /* Local buffer and variables for time tracking */
  char uptime_buf[17];
  uint32_t sec = 0, min = 0, hour = 0;

  /* Initialize communication handle with global data pointers */
  Comm_Handle_t hGatewayComm = {
      .pRxBuffer     = Global_t.uart_gateway.rx_buffer,
      .pErrorCounter = &Global_t.uart_gateway.uart_error_cnt
  };

  /* Start background UART reception via DMA */
  HAL_UART_Receive_DMA(&huart3, Global_t.uart_gateway.rx_buffer, FRAME_LENGTH);

  /* Enable UART Idle Line detection interrupt */
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

  /* Initial display setup */
  LCD_Clear();

  for(;;)
  {
    /* Current timestamp snapshot */
    uint32_t current_tick = HAL_GetTick();

    /* --- SECTION 1: PACKET ACQUISITION --- */
    /* Priority: Always check for incoming data frames first */
    if ( Global_t.uart_gateway.packet_ready == 1 )
    {
      /* Parse packet and set the display_timeout_flag to 1 */
      Comm_Process_Packet(&hGatewayComm);

      /* Reset ISR flag after processing */
      Global_t.uart_gateway.packet_ready = 0;
    }

    /* --- SECTION 2: UI STATE MACHINE --- */
    /* Check if the system is in 'Transient Message' or 'Idle Monitoring' state */
    if (Global_t.uart_gateway.display_timeout_flag == 1)
    {
        /* STATE: HOLD - Maintain packet info on screen for 3 seconds */
        if (current_tick - Global_t.uart_gateway.last_packet_tick > 3000)
        {
            /* Transition to IDLE: Reset flag and clear screen for uptime telemetry */
            Global_t.uart_gateway.display_timeout_flag = 0;
            LCD_Clear();
        }
    }
    else
    {
        /* STATE: IDLE - Routine System Uptime and Status Update */
        sec  = (current_tick / 1000) % 60;
        min  = (current_tick / 60000) % 60;
        hour = (current_tick / 3600000);

        /* Format uptime string for row 0 */
        sprintf(uptime_buf, "UPTIME: %02lu:%02lu:%02lu", hour, min, sec);

        LCD_Put_Cursor(0, 0);
        LCD_Send_String(uptime_buf);

        /* Fixed status message for row 1 */
        LCD_Put_Cursor(1, 0);
        LCD_Send_String("IDLE - READY    ");
    }

    /* Maintain task frequency at 10Hz to ensure UI stability */
    osDelay(100);
  }
  /* USER CODE END StartCommunicationTask */
}


/* USER CODE END Application */

