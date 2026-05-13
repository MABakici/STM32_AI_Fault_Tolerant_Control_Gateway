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
  /* --- Local Variables & Buffer Initialization --- */
  char line_buf[21];
  uint32_t sec = 0, min = 0, hour = 0;

  /* Configure communication handle with global gateway references */
  Comm_Handle_t hGatewayComm = {
      .pRxBuffer     = Global_t.uart_gateway.rx_buffer,
      .pErrorCounter = &Global_t.uart_gateway.uart_error_cnt
  };

  /* Initiate asynchronous background data reception via DMA */
  HAL_UART_Receive_DMA(&huart3, Global_t.uart_gateway.rx_buffer, FRAME_LENGTH);

  /* Perform initial hardware reset for the display unit */
  LCD_Clear();
  osDelay(100);

  for(;;)
  {
    uint32_t current_tick = HAL_GetTick();

    /* --- ROW 0: SYSTEM UPTIME TELEMETRY --- */
    sec  = (current_tick / 1000) % 60;
    min  = (current_tick / 60000) % 60;
    hour = (current_tick / 3600000);

    /* Format uptime string with fixed width to prevent character ghosting */
    sprintf(line_buf, "UPTIME: %02lu:%02lu:%02lu    ", hour, min, sec);
    LCD_Put_Cursor(0, 0);
    LCD_Send_String(line_buf);

    /* --- ROW 1: UART CHANNEL MONITORING --- */
    /* Process incoming frames if the Packet Ready flag is set by ISR */
    if (Global_t.uart_gateway.packet_ready == 1) {
        Comm_Process_Packet(&hGatewayComm);
        Global_t.uart_gateway.packet_ready = 0;
    }

    LCD_Put_Cursor(1, 0);
    if (Global_t.uart_gateway.display_timeout_flag == 1) {
        /* Maintain transient packet notification for defined timeout period */
        LCD_Send_String("UART: PKT RECEIVED  ");
    } else {
        LCD_Send_String("UART: LISTENING     ");
    }

    /* --- ROW 2: IMU SENSOR FUSION DATA --- */
    /* Update display based on safety flags generated by Analysis Task */
    LCD_Put_Cursor(2, 0);
    if (Global_t.is_tilt_detected) {
        LCD_Send_String("IMU: TILT DETECTED  ");
    } else if (Global_t.is_impact_detected) {
        LCD_Send_String("IMU: IMPACT DETECTED");
    } else {
        LCD_Send_String("IMU: SYSTEM STABLE  ");
    }

    /* --- ROW 3: MOTOR ACTUATOR STATUS --- */
    /* Reflect real-time output state of the DRV8833 driver */
    LCD_Put_Cursor(3, 0);
    if (Global_t.is_tilt_detected) {
        LCD_Send_String("MOT: HALTED         ");
    } else {
        LCD_Send_String("MOT: ACTIVE (50%)   ");
    }

    /* Task frequency set to 4Hz to minimize I2C bus load while maintaining readability */
    osDelay(250);
  }
}


/**
 * @brief  Task: Motor Actuation & Control (Normal Priority)
 * @author Mehmet Alperen BAKICI
 * @details Manages DRV8833 motor states based on system flags or telemetry.
 */
void StartMotorCntTask(void *argument)
{
  /* The driver is already initialized in AI_Based_Fault_Tolerant_Control_Gateway_Init */

  for(;;)
  {
    /* Example Logic: If tilt is detected, stop the motor for safety */
    if (Global_t.is_tilt_detected)
    {
      DRV8833_Set_Motor(MOTOR_A, MOTOR_STOP, 0);
    }
    else
    {
      /* Standard Operation: Forward at 50% speed */
      DRV8833_Set_Motor(MOTOR_A, MOTOR_FORWARD, 50);
    }

    /* 20Hz Control Loop Frequency (50ms) is ideal for DC motor response */
    osDelay(50);
  }
}


/* USER CODE END Application */

