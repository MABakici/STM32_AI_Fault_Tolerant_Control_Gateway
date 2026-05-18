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
#include "control_gateway_drv8833_driver.h"
#include "cmsis_os2.h"
#include "math.h"
#include "mpu6050.h"
#include "global_variables_CM7.h"
#include "control_gateway_comm_handler.h"
#include "control_gateway_sensor_hub.h"
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
 * @brief  Task 3: System Heartbeat & Climate Monitoring (Low Priority)
 * @author Mehmet Alperen BAKICI
 * @details Toggles Yellow LED at 1Hz and safely polls DHT11 climate metrics
 * at a deterministic 1Hz frequency using the new modular sensor hub driver.
 */
void StartHeartbeatTask(void *argument)
{
  float temp_c = 0.0f;
  float hum_pct = 0.0f;

  /* CRITICAL: Must be static to persist across FreeRTOS context switches */
  static uint8_t dht11_timer_cnt = 0;

  for(;;)
  {
    /* --- 1. LED TOGGLE (Heartbeat) --- */
    if ( Global_t.mpu6050_veriler.is_initialized )
    {
      HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
    }

    /* --- 2. DETERMINISTIC 1Hz DHT11 POLLING --- */
    dht11_timer_cnt++;
    if (dht11_timer_cnt >= 2) /* 2 loops * 500ms = 1000ms (1Hz) */
    {
      if (DHT11_Read_Raw(&temp_c, &hum_pct))
      {
        Global_t.SensorHub_t.temperature_C = temp_c;
        Global_t.SensorHub_t.humidity_pct  = hum_pct;
      }
      dht11_timer_cnt = 0;
    }

    osDelay(500);
  }
}

/**
 * @brief  Task: Communication & UI Gateway (Normal Priority)
 * @author Mehmet Alperen BAKICI
 * @version v1.0.0 (Fixed-Grid Industrial UI Production Baseline)
 * @date   19.05.2026
 * @details Handles UART DMA packet processing and orchestrates the 20x4 LCD
 * telemetry display with pixel-perfect vertical layout alignment.
 * Fixed-grid system locks vertical separators (|) strictly at column 12.
 */
void StartCommTask(void *argument)
{
    char line_buf[32]; // Hardened layout buffer against format-overflow constraints
    uint32_t sec = 0, min = 0, hour = 0;
    Comm_Handle_t hGatewayComm = {
        .pRxBuffer     = Global_t.uart_gateway.rx_buffer,
        .pErrorCounter = &Global_t.uart_gateway.uart_error_cnt
    };

    /* Start UART ring buffer pipeline via DMA link */
    HAL_UART_Receive_DMA(&huart3, Global_t.uart_gateway.rx_buffer, FRAME_LENGTH);
    LCD_Clear();
    osDelay(100);

    for(;;)
    {
        uint32_t current_tick = HAL_GetTick();

        /* --- 1. UART PACKET PROCESSING ENGINE --- */
        if (Global_t.uart_gateway.packet_ready == 1)
        {
            Comm_Process_Packet(&hGatewayComm);

            /* Update telemetry timestamp and fire visual lock flag */
            Global_t.uart_gateway.last_packet_tick = current_tick;
            Global_t.uart_gateway.display_timeout_flag = 1;

            Global_t.uart_gateway.packet_ready = 0;
        }

        /* --- 2. ROW 0: UPTIME & TEMPERATURE TELEMETRY --- */
        sec = (current_tick / 1000) % 60;
        min = (current_tick / 60000) % 60;
        hour = (current_tick / 3600000);

        /* Left side: 12 chars, Separator: 1 char, Right side: 7 chars = Exactly 20 chars */
        sprintf(line_buf, "UPT:%02lu:%02lu:%02lu| T:%2ldC ",
                hour, min, sec,
                (int32_t)Global_t.SensorHub_t.temperature_C);
        LCD_Put_Cursor(0, 0);
        LCD_Send_String(line_buf);

        /* --- 3. ROW 1: UART NETWORK MONITORING & VISUAL LOCK --- */
        LCD_Put_Cursor(1, 0);
        if (Global_t.uart_gateway.display_timeout_flag == 1)
        {
            if (current_tick - Global_t.uart_gateway.last_packet_tick < 3000)
            {
                LCD_Send_String("UART:PKT RECEIVED   "); // Exactly 20 characters
            }
            else
            {
                Global_t.uart_gateway.display_timeout_flag = 0;
                LCD_Send_String("UART:LISTENING      "); // Exactly 20 characters
            }
        }
        else
        {
            LCD_Send_String("UART:LISTENING      "); // Exactly 20 characters
        }

        /* --- 4. ROW 2: IMU STATUS & HUMIDITY TELEMETRY --- */
        LCD_Put_Cursor(2, 0);

        /* %-12s guarantees that left token is left-padded to 12 chars dynamically */
        if (Global_t.is_tilt_detected)
        {
            sprintf(line_buf, "%-12s| H:%2ld%% ", "IMU:TILT", (int32_t)Global_t.SensorHub_t.humidity_pct);
        }
        else if (Global_t.is_impact_detected)
        {
            sprintf(line_buf, "%-12s| H:%2ld%% ", "IMU:IMPACT", (int32_t)Global_t.SensorHub_t.humidity_pct);
        }
        else
        {
            sprintf(line_buf, "%-12s| H:%2ld%% ", "IMU:STABLE", (int32_t)Global_t.SensorHub_t.humidity_pct);
        }
        LCD_Send_String(line_buf);

        /* --- 5. ROW 3: MOTOR ACTUATOR & CURRENT TRANSIENT MONITORING --- */
        LCD_Put_Cursor(3, 0);

        // State Alpha: Fault Condition (Overcurrent Overload)
        if (Global_t.SensorHub_t.overcurrent_flag == 1)
        {
            sprintf(line_buf, "%-12s|%4ldmA", "ERR:OVERCUR!", (int32_t)Global_t.SensorHub_t.current_mA);
        }
        // State Beta: Safety Shutdown (System Halted via Flight Interlock)
        else if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
        {
            sprintf(line_buf, "%-12s|%4ldmA", "MOT:HLT", (int32_t)Global_t.SensorHub_t.current_mA);
        }
        // State Gamma: Nominal Execution Mode
        else
        {
            char speed_str[16];
            sprintf(speed_str, "MOT:%3d%%", Global_t.dc_motor_control.target_speed);
            sprintf(line_buf, "%-12s|%4ldmA", speed_str, (int32_t)Global_t.SensorHub_t.current_mA);
        }
        LCD_Send_String(line_buf);

        /* Deterministic 5Hz interface refresh grid pace */
        osDelay(200);
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



/**
 * @brief  Task: Sensor Fusion & Environmental Monitoring (Low Priority)
 * @author Mehmet Alperen BAKICI
 * @version v0.0.8 (Production Baseline - Heavy Cascaded Filter)
 * @date    19.05.2026
 * @details Implements a stable 3-stage cascaded digital filter (Strict Median +
 * 16-point Moving Average + Constant Heavy EMA) to eliminate erratic fırça spikes.
 * Fully hardfault protected and synchronized with power-on calibration.
 */
void StartSensorFusion(void *argument)
{
    /* Hardware and Signal Conversion Constants */
    const float ADC_TO_VOLT = 3.3f / 65535.0f;
    const float SENSITIVITY = 0.185f; /* ACS712 5A Module Sensitivity (185mV/A) */
    const float GAIN_FACTOR = 2.3f;   /* Empirical calibration gain factor */

    /* Memory-Isolated Local Static Variables */
    static float local_v_offset;
    static float local_smoothed_current = 0.0f;
    static uint8_t calibration_done_flag = 0;

    /* Pull the fresh baseline established by the synchronous Power-On initialization */
    local_v_offset = Global_t.SensorHub_t.v_offset;

    /* Initialization Guard Delay */
    osDelay(500);

    /* 16-Element Signal Smoothing Ring Buffer Configuration */
    #define MOVING_AVG_SIZE 16
    float moving_avg_buffer[MOVING_AVG_SIZE] = {0.0f};
    uint8_t avg_index = 0;
    float moving_avg_sum = 0.0f;

    /* CONSTANT HEAVY EMA LAYER 🚀
     * Locked to 0.07f to permanently suppress brushed DC commutation ripple
     * and avoid self-triggering false current spikes during nominal flight. */
    const float EMA_ALPHA_HEAVY = 0.07f;

    for(;;)
    {
        uint32_t samples[15];

        /* STEP 1: Burst Analog Data Acquisition via Shared DMA Buffer */
        for(int i = 0; i < 15; i++)
        {
            samples[i] = (uint16_t)Global_t.SensorHub_t.adc_buffer[0];
            osDelay(1);
        }

        /* STEP 2: Non-Linear Median Filtering (Bubble Sort Optimization) */
        for(int i = 0; i < 14; i++)
        {
            for(int j = i + 1; j < 15; j++)
            {
                if(samples[i] > samples[j])
                {
                    uint32_t temp = samples[i];
                    samples[i] = samples[j];
                    samples[j] = temp;
                }
            }
        }

        /* STEP 3: Strict Median Extraction
         * Extract exactly the center sample (Index 7) to block asymmetrical spikes. */
        float filtered_raw_adc = (float)samples[7];
        float current_voltage = filtered_raw_adc * ADC_TO_VOLT;

        /* STEP 4: Dynamic Continuous Gated-Calibration Execution */
        if (Global_t.is_tilt_detected == 1)
        {
            if (calibration_done_flag == 0)
            {
                float dyn_offset_sum = 0.0f;
                for(int c = 0; c < 30; c++)
                {
                    dyn_offset_sum += (float)((uint16_t)Global_t.SensorHub_t.adc_buffer[0]) * ADC_TO_VOLT;
                    osDelay(5);
                }
                local_v_offset = dyn_offset_sum / 30.0f;
                calibration_done_flag = 1;
            }
        }
        else
        {
            calibration_done_flag = 0;
        }

        /* STEP 5: Mathematical Current Computation with Direction Guard */
        float delta_v = local_v_offset - current_voltage;
        if(delta_v < 0.0f) { delta_v = -delta_v; }

        float current_calculated = (delta_v / SENSITIVITY) * 1000.0f * GAIN_FACTOR;

        /* STEP 6: Input-Stage Software Blanking (Dead-Band Budding) */
        if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
        {
            current_calculated = 0.0f;
        }
        else if (current_calculated < 45.0f)
        {
            current_calculated = 0.0f;
        }

        /* STEP 7: Bound-Protected Moving Average Ring Buffer Execution */
        moving_avg_sum -= moving_avg_buffer[avg_index];
        moving_avg_buffer[avg_index] = current_calculated;
        moving_avg_sum += moving_avg_buffer[avg_index];

        avg_index = (avg_index + 1) & 15;
        float moving_avg_result = moving_avg_sum / 16.0f;

        /* STEP 8: Constant Heavy Smoothing Execution */
        local_smoothed_current = (EMA_ALPHA_HEAVY * moving_avg_result) + ((1.0f - EMA_ALPHA_HEAVY) * local_smoothed_current);

        /* Hardware Fail-Safe Interlocking Override */
        if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
        {
            local_smoothed_current = 0.0f;
        }

        /* STEP 9: Telemetry Export to Global Shared Memory State */
        Global_t.SensorHub_t.current_mA = local_smoothed_current;
        Global_t.SensorHub_t.v_offset   = local_v_offset;

        if (Global_t.SensorHub_t.current_mA > 1500.0f) {
            Global_t.SensorHub_t.overcurrent_flag = 1;
        } else {
            Global_t.SensorHub_t.overcurrent_flag = 0;
        }

        /* STEP 10: Deterministic 1Hz Industrial UART Data Logging (Extended CSV Stream) */
        static uint8_t log_counter = 0;
        log_counter++;

        if (log_counter >= 15) /* Calibrated to achieve precise ~1Hz streaming */
        {
        	char uart_buf[128]; // Expanded buffer size for extended dataset

            /* Extended Format for TinyML: [Current_mA],[Offset_mV],[Tilt_Flag],[Temp_C],[Hum_Pct]
             * All formats converted to %ld to perfectly match (int32_t) casting. */
            int uart_len = sprintf(uart_buf, "%ld,%ld,%ld,%ld,%ld\r\n",
                                   (int32_t)Global_t.SensorHub_t.current_mA,
                                   (int32_t)(local_v_offset * 1000.0f),
                                   (int32_t)Global_t.is_tilt_detected,
                                   (int32_t)Global_t.SensorHub_t.temperature_C,
                                   (int32_t)Global_t.SensorHub_t.humidity_pct);

            HAL_UART_Transmit(&huart3, (uint8_t*)uart_buf, uart_len, 15);
            log_counter = 0;
        }

        /* Task Periodicity Delay */
        osDelay(50);
    }
}

/* USER CODE END Application */

