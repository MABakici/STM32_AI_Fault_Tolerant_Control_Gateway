/**
 * @file    drv8833_driver.h
 * @author  Mehmet Alperen BAKICI
 * @brief   Header file for DRV8833 Dual H-Bridge Motor Driver.
 * @date    14.05.2026
 */

#ifndef CONTROL_GATEWAY_DRV8833_DRIVER_H_
#define CONTROL_GATEWAY_DRV8833_DRIVER_H_

#include "main.h"
#include <control_gateway_comm.h>

/* Motor Control States */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD,
    MOTOR_COAST
} Motor_State_t;

/* Motor Selection (For Dual Motor Applications) */
typedef enum {
    MOTOR_A = 0,
    MOTOR_B
} Motor_ID_t;

/**
 * @brief  Initializes the DRV8833 by linking the timer and starting PWM.
 * @param  htim: Pointer to the Timer Handle (e.g., &htim3)
 */
void DRV8833_Init(TIM_HandleTypeDef *htim);

/**
 * @brief  Updates the direction and speed of the selected motor.
 * @param  motor: Selected motor (MOTOR_A or MOTOR_B)
 * @param  state: Desired movement state
 * @param  speed: PWM Duty Cycle as a percentage (0 to 100)
 */
void DRV8833_Set_Motor(Motor_ID_t motor, Motor_State_t state, uint8_t speed);

#endif /* CONTROL_GATEWAY_DRV8833_DRIVER_H_ */
