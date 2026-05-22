/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications (v0.2.0 TTC Core Baseline)
  * @author            : Mehmet Alperen BAKICI
  * @date              : 2026.05.22
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
#include "semphr.h"

extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart3;

/* Realigned with official CubeMX hardware peripheral naming convention */
extern IWDG_HandleTypeDef hiwdg1;
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
volatile uint8_t g_wdt_test_trigger = 0;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Process_Motor_Current_Resolution(void);
/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void MX_FREERTOS_Init(void) {
    /* USER CODE BEGIN RTOS_SEMAPHORES */

    /* Allocate and Map Binary Semaphores directly into the Global Architecture */
    Global_t.TaskMgmt_t.sem_1000Hz = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_500Hz  = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_250Hz  = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_100Hz  = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_50Hz   = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_25Hz   = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_10Hz   = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_5Hz    = xSemaphoreCreateBinary();
    Global_t.TaskMgmt_t.sem_1Hz    = xSemaphoreCreateBinary();

    /* USER CODE END RTOS_SEMAPHORES */
}

/**
 * @brief  Task Slot: 1000Hz (1ms Periodicity) - Realtime / Safety-Critical
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.20
 * @details Pure empty hardware-paced slots controlled by TIM5 master sync.
 */
void Start_1000Hz_Task(void *argument)
{
    for(;;)
    {
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_1000Hz, portMAX_DELAY) == pdTRUE)
        {
            /* Room 1000Hz: Pure Empty reserved for hard real-time execution */
        }
    }
}

/**
 * @brief  Task Slot: 500Hz (2ms Periodicity) - Ultra-High Speed
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.20
 * @details Pure empty hardware-paced slots controlled by TIM5 master sync.
 */
void Start_500Hz_Task(void *argument)
{
    for(;;)
    {
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_500Hz, portMAX_DELAY) == pdTRUE)
        {
            /* Room 500Hz: Pure Empty reserved for high-frequency signal monitoring */
        }
    }
}

/**
 * @brief  Task Slot: 250Hz (4ms Periodicity) - High Speed Actuation
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.20
 * @details Pure empty hardware-paced slots controlled by TIM5 master sync.
 */
void Start_250Hz_Task(void *argument)
{
    for(;;)
    {
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_250Hz, portMAX_DELAY) == pdTRUE)
        {
            /* Room 250Hz: Pure Empty reserved for high-speed dynamic control */
        }
    }
}

/**
 * @brief  Task Slot: 100Hz (10ms Periodicity) - Sensor Ingestion Hub
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.20
 * @details Reads raw data from MPU6050 via I2C with Mutex protection and
 * immediately runs deterministic tilt and impact analysis at 100Hz.
 */
void Start_100Hz_Task(void *argument)
{
    /* Local counter for physical tilt debouncing */
    uint16_t tilt_filter_cnt = 0;

    for(;;)
    {
        /* Strict 10ms hardware pace triggered from TIM5 ISR */
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_100Hz, portMAX_DELAY) == pdTRUE)
        {
            Global_t.TaskMgmt_t.debug_100Hz_cnt++;

            /* --- STEP 1: MPU6050 DATA ACQUISITION --- */
            if (Global_t.mpu6050_veriler.is_initialized)
            {
                /* Thread-safe I2C Bus Resource Protection via Mutex */
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
                    /* Promptly release the shared I2C peripheral */
                    osMutexRelease(I2C2_MutexHandle);
                }
            }

            /* --- STEP 2: REALTIME SENSOR ANALYSIS --- */
            if (Global_t.mpu6050_veriler.is_initialized && Global_t.i2c_error_cnt == 0)
            {
                float ax = Global_t.mpu6050_veriler.Accel_X;
                float ay = Global_t.mpu6050_veriler.Accel_Y;
                float az = Global_t.mpu6050_veriler.Accel_Z;

                /* 2.1: Deterministic Impact / G-Force Anomaly Detection */
                float total_g = sqrtf((ax * ax) + (ay * ay) + (az * az));

                if (total_g > 1.5f || total_g < 0.5f)
                {
                    Global_t.is_impact_detected = 1;
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); /* Fire Warning/Impact LED */
                }
                else
                {
                    Global_t.is_impact_detected = 0;
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                }

                /* 2.2: Deterministic Tilt Detection (az < 0.65f represents ~50 deg tilt) */
                if (az < 0.65f)
                {
                    /* Debounce tracking: Trigger after 5 consecutive 100Hz samples (50ms total) */
                    if (++tilt_filter_cnt > 5)
                    {
                        Global_t.is_tilt_detected = 1;
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); /* Fire Safety/Tilt LED */
                    }
                }
                else
                {
                    tilt_filter_cnt = 0;
                    Global_t.is_tilt_detected = 0;
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                }
            }
        }
    }
}

/**
 * @brief  Task Slot: 50Hz (20ms Periodicity) - Motor Actuation & Safety Control
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.20
 * @details Implements a hardware-paced, safety-first motor control logic at 50Hz.
 * Prioritizes immediate cessation (Tilt/Halt Interlocks) over nominal PWM driving.
 */
void Start_50Hz_Task(void *argument)
{
    /* Initialize actuator defaults within the task context to ensure predictable states */
    Global_t.dc_motor_control.target_speed = 50;
    Global_t.dc_motor_control.is_system_halted = 0;

    for(;;)
    {
        /* Strict 20ms hardware cadence enforced by TIM5 master clock semaphore */
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_50Hz, portMAX_DELAY) == pdTRUE)
        {
            Global_t.TaskMgmt_t.debug_50Hz_cnt++;
            /**
             * @section Safety Interlock Evaluation
             * Immediate motor shutdown triggered if physical tilt failure is active
             * OR software emergency flight stop (is_system_halted) is fired.
             */
            if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
            {
                /* Force immediate physical actuator cessation */
                DRV8833_Set_Motor(MOTOR_A, MOTOR_STOP, 0);
            }
            else
            {
                /**
                 * @section Nominal Operational Mode
                 * Inject the target speed duty cycle directly into the DRV8833 driver.
                 */
                DRV8833_Set_Motor(MOTOR_A, MOTOR_FORWARD, Global_t.dc_motor_control.target_speed);
            }
        }
    }
}

/**
 * @brief  Task Slot: 10Hz (100ms Periodicity) - UI Refresh & Network Ingestion
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.21
 * @details Handles real-time asynchronous UART command ingestion and packet dispatching at
 * a native 10Hz cadence, while executing a deterministic 2:1 downsampling (decimation)
 * gate to enforce a stabilized 5Hz refresh cycle on the liquid crystal display (LCD).
 * @note    The telemetry tracking counters are strictly handled outside sub-routines to avoid
 * algorithmic decimation lag during context switching.
 * @param  argument: Not used
 * @retval None
 */
void Start_10Hz_Task(void *argument)
{
    /* Local string buffer for display layout formatting */
    char line_buf[32];
    uint32_t sec = 0, min = 0, hour = 0;

    /* Fixed frequency divider used to decimate the 10Hz loop cadence into a 5Hz LCD update rate */
    static uint8_t lcd_refresh_divider = 0;

    /* Bind communication context handle explicitly with the global peripheral structure */
    Comm_Handle_t hGatewayComm = {
        .pRxBuffer     = Global_t.uart_gateway.rx_buffer,
        .pErrorCounter = &Global_t.uart_gateway.uart_error_cnt
    };

    /* Initialize physical screen layout on boot phase */
    LCD_Clear();

    for(;;)
    {
        /* Enforced periodic cadence from the TIM5 hardware master interrupt via semaphore pipeline */
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_10Hz, portMAX_DELAY) == pdTRUE)
        {
            Global_t.TaskMgmt_t.debug_10Hz_cnt++;

            /* Cache real-time system clock reference locally */
            uint32_t current_tick = Global_t.TaskMgmt_t.system_master_tick;

            /* --- STEP 1: REAL-TIME UART PACKET PROCESSING (10Hz Low-Latency Domain) --- */
            if (Global_t.uart_gateway.packet_ready == 1)
            {
                Comm_Process_Packet(&hGatewayComm);
                Global_t.uart_gateway.last_packet_tick = current_tick;
                Global_t.uart_gateway.display_timeout_flag = 1;
                Global_t.uart_gateway.packet_ready = 0;
            }

            /* --- STEP 2: AMBIENT LIGHT SENSOR DATA INGESTION --- */
            if (HAL_GPIO_ReadPin(LDR_PIN_GPIO_Port, LDR_PIN_Pin) == GPIO_PIN_SET)
            {
                Global_t.SensorHub_t.is_dark = 1;
            }
            else
            {
                Global_t.SensorHub_t.is_dark = 0;
            }

            /* --- STEP 3: LCD DETERMINISTIC 5Hz DECIMATION GATE ---
             * Dividing the underlying 10Hz scheduler pace strictly by a factor of 2.
             * Executes display bus refresh every 200ms to preserve system bus efficiency. */
            lcd_refresh_divider++;
            if (lcd_refresh_divider >= 2)
            {
                /* --- DISPLAY MATRIX ROW 0: SYSTEM UPTIME & THERMAL TELEMETRY --- */
                sec  = (current_tick / 1000) % 60;
                min  = (current_tick / 60000) % 60;
                hour = (current_tick / 3600000);

                sprintf(line_buf, "UPT:%02lu:%02lu:%02lu| T:%2ldC ", hour, min, sec, (int32_t)Global_t.SensorHub_t.temperature_C);
                LCD_Put_Cursor(0, 0);
                LCD_Send_String(line_buf);

                /* --- DISPLAY MATRIX ROW 1: COMMUNICATION GATEWAY STATUS & OPTICAL STATE --- */
                LCD_Put_Cursor(1, 0);
                const char* ldr_str = (Global_t.SensorHub_t.is_dark == 1) ? "L:DRK" : "L:LGT";

                if (Global_t.uart_gateway.display_timeout_flag == 1)
                {
                    /* Maintain message retention on the screen for a strict 3000ms duration window */
                    if (current_tick - Global_t.uart_gateway.last_packet_tick < 3000)
                    {
                        sprintf(line_buf, "%-12s| %s ", "UART:RX_PACK", ldr_str);
                    }
                    else
                    {
                        Global_t.uart_gateway.display_timeout_flag = 0;
                        sprintf(line_buf, "%-12s| %s ", "UART:LISTEN", ldr_str);
                    }
                }
                else
                {
                    sprintf(line_buf, "%-12s| %s ", "UART:LISTEN", ldr_str);
                }
                LCD_Send_String(line_buf);

                /* --- DISPLAY MATRIX ROW 2: INERTIAL MEASUREMENT UNIT (IMU) FAULT ISOLATION --- */
                LCD_Put_Cursor(2, 0);
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

                /* --- DISPLAY MATRIX ROW 3: ACTUATOR VELOCITY & DIGITAL RESOLVER CURRENT --- */
                LCD_Put_Cursor(3, 0);
                if (Global_t.SensorHub_t.overcurrent_flag == 1)
                {
                    sprintf(line_buf, "%-12s|%4ldmA", "ERR:OVERCUR!", (int32_t)Global_t.SensorHub_t.current_mA);
                }
                else if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
                {
                    sprintf(line_buf, "%-12s|%4ldmA", "MOT:HLT", (int32_t)Global_t.SensorHub_t.current_mA);
                }
                else
                {
                    char speed_str[16];
                    sprintf(speed_str, "MOT:%3d%%", Global_t.dc_motor_control.target_speed);
                    sprintf(line_buf, "%-12s|%4ldmA", speed_str, (int32_t)Global_t.SensorHub_t.current_mA);
                }
                LCD_Send_String(line_buf);

                /* Reset the downsampling phase divider accumulator */
                lcd_refresh_divider = 0;
            }
        }
    }
}

/**
 * @brief  Task Slot: 5Hz (200ms Periodicity) - Climate Ingestion & Current Resolution
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.22
 * @details Deterministic multirate pipeline driving DHT11 climate sampling via 1Hz
 * decimation and high-performance DC motor telemetry calculations at 5Hz.
 */
void Start_5Hz_Task(void *argument)
{
    float temp_c = 0.0f;
    float hum_pct = 0.0f;
    static uint8_t dht11_decimation_cnt = 0;

    for(;;)
    {
        /* Enforced periodic cadence from the TIM5 hardware master interrupt via semaphore pipeline */
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_5Hz, portMAX_DELAY) == pdTRUE)
        {
            Global_t.TaskMgmt_t.debug_5Hz_cnt++;

            /* --- SUB-ROUTINE A: RESOLVE CURRENT AND EXHAUST DSP PIPELINE (5Hz Rate) --- */
            Process_Motor_Current_Resolution();

            /* Advance the decimation timeline index for climate sensor */
            dht11_decimation_cnt++;

            /* 1Hz Gate Check: Execute physical acquisition once every 5 framework frames (1000ms) */
            if (dht11_decimation_cnt >= 5)
            {
                /* --- CRITICAL SAFE ZONE: CAPTURE LOCKOUT --- */
                vTaskSuspendAll();

                /* Fire non-blocking low-level bit-banging capture on the 1-Wire physical bus */
                uint8_t dht11_success = DHT11_Read_Raw(&temp_c, &hum_pct);

                xTaskResumeAll();

                /* --- DATA VERIFICATION & HARDWARE INGESTION GATE --- */
                if (dht11_success == 1 && temp_c > 0.1f && hum_pct > 0.1f && temp_c < 80.0f)
                {
                    /* Commit verified metrics securely to the tightly packed global context */
                    Global_t.SensorHub_t.temperature_C = temp_c;
                    Global_t.SensorHub_t.humidity_pct  = hum_pct;
                }

                /* Clear the decimation tracking bucket for the subsequent 1000ms period */
                dht11_decimation_cnt = 0;
            }
        }
    }
}

/**
 * @brief  Task Slot: 1Hz (1000ms Periodicity) - Diagnostics & Network Telemetry
 * @author Mehmet Alperen BAKICI
 * @date   2026.05.21
 * @details Toggles the system heartbeat LED and streams highly synchronized
 * telemetry dataset via UART for edge AI / TinyML model ingestion at a strict 1Hz cadence.
 */
void Start_1Hz_Task(void *argument)
{
	char diagnostic_uart_buf[160];
	    int diagnostic_msg_len = 0;
    for(;;)
    {
        /* Using sem_1Hz ensures the LED toggles and telemetry transmits exactly once per second. */
        if (xSemaphoreTake(Global_t.TaskMgmt_t.sem_1Hz, portMAX_DELAY) == pdTRUE)
        {
            Global_t.TaskMgmt_t.debug_1Hz_cnt++;

            /* --- STEP 1: SYSTEM HEARTBEAT INDICATOR --- */
            if (Global_t.mpu6050_veriler.is_initialized)
            {
                /* Toggles cleanly at exactly 1Hz pace */
                HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
            }

            /* --- STEP 2: CRITICAL FAULT INJECTION TEST CASE ---
             * When g_wdt_test_trigger is manually evaluated to 1 via STM32CubeIDE Live Expressions,
             * this thread forces an intentional infinite loop. This starves the hardware refresh
             * signal and validates the Independent Watchdog (IWDG) automated recovery mechanism. */
            if (g_wdt_test_trigger == 1)
            {
                diagnostic_msg_len = sprintf(diagnostic_uart_buf,
                    "\r\n[WARNING] Watchdog fault condition injected. Starving IWDG reload boundary. Hardware reset pending...\r\n");

                /* Iterate through the string buffer explicitly at register level */
                for (int idx = 0; idx < diagnostic_msg_len; idx++)
                {
                    /* Hardware Spinlock: Wait strictly until TDR (Transmit Data Register) is empty.
                     * TXE (Transmit Data Register Empty) flag resides in the USART ISR (Interrupt & Status Register). */
                    while ((huart3.Instance->ISR & USART_ISR_TXE_TXFNF) == 0)
                    {
                        __asm volatile("" : : : "memory");
                    }

                    /* Inject character directly into the physical hardware register */
                    huart3.Instance->TDR = (uint8_t)diagnostic_uart_buf[idx];
                }

                /* Hardware Flush: Wait until Transmission Complete (TC) flag is physically set in the silicon */
                while ((huart3.Instance->ISR & USART_ISR_TC) == 0)
                {
                    __asm volatile("" : : : "memory");
                }

                /* Enforce atomic instruction/data synchronization boundaries */
                __DSB();
                __ISB();

                /* 👑 SAFE SOFTWARE DELAY:
                 * Bypasses HAL_Delay completely to circumvent the custom TIM5 / HAL_GetTick conflict.
                 * Executes a pure assembly-backed loop to let the UART hardware pins flush fully before the trap. */
                for (volatile uint32_t delay_cycle = 0; delay_cycle < 16000000; delay_cycle++)
                {
                    __NOP(); /* Burn raw CPU cycles safely at 400+ MHz */
                }

                while(1)
                {
                    /* Deliberately trap the execution pipeline to enforce an automated hardware recovery */
                    __NOP();
                }
            }

            /* --- STEP 3: INDEPENDENT WATCHDOG (IWDG) SERVICE GATING ---
             * Linked explicitly to the active CubeMX peripheral handle instance (hiwdg1). */
            HAL_IWDG_Refresh(&hiwdg1);

            /* --- STEP 4: PRODUCTION TELEMETRY & TINYML DATA STREAM --- */
            /* Authentic Clean Format for Edge AI Dataset Processing:
             * [Smoothed_Current_mA],[Dynamic_Offset_mV],[Temperature_C],[Humidity_Pct] */

            /* Note: Uncomment below for real-time edge streaming data payload activation
            char uart_buf[160];
            int uart_len = sprintf(uart_buf, "%ld mA, %ld mV, T:%ld C, H:%%%ld\r\n",
                                   (int32_t)Global_t.SensorHub_t.current_mA,
                                   (int32_t)(Global_t.SensorHub_t.v_offset * 1000.0f),
                                   (int32_t)Global_t.SensorHub_t.temperature_C,
                                   (int32_t)Global_t.SensorHub_t.humidity_pct);

            HAL_UART_Transmit(&huart3, (uint8_t*)uart_buf, uart_len, 30);
            */
        }
    }
}

/* USER CODE END Application */
