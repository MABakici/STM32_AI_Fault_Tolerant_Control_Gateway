#ifndef GLOBAL_VARIABLES_CM7_H_
#define GLOBAL_VARIABLES_CM7_H_

#include <control_gateway_definitions.h>
#include "MPU6050.h"
#include <stdint.h>


/**
 * @brief  Global Data Management Structure
 * @note   Centralized container for inter-task communication and state monitoring.
 */
typedef struct
{
    MPU6050_t       mpu6050_veriler;      /* IMU raw and processed data */
    uint8_t         is_tilt_detected;     /* Tilt threshold safety flag */
    uint8_t         is_impact_detected;   /* Impact/G-Force event flag */
    uint32_t        i2c_error_cnt;        /* Communication fault counter */

    uint32_t        system_heartbeat;     /* Task scheduler health indicator */

    UART_Comm_t     uart_gateway;         /* Asynchronous communication handler */
    Motor_Control_t dc_motor_control;     /* Actuator state and telemetry handle */

} Global_Variables_t;


extern Global_Variables_t Global_t;

#endif
