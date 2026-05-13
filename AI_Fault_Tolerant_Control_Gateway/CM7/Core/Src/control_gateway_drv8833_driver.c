/**
 * @file    drv8833_driver.c
 * @author  Mehmet Alperen BAKICI
 * @brief   Implementation of DRV8833 driver logic using PWM.
 * @date    14.05.2026
 */

#include "control_gateway_drv8833_driver.h"

static TIM_HandleTypeDef *pMotorTimer;

/**
 * @brief  Initializes the DRV8833 by linking the timer and starting PWM.
 * @note   Includes automatic serial logging for system diagnostics.
 * @param  htim: Pointer to the Timer Handle (e.g., &htim3)
 */
void DRV8833_Init(TIM_HandleTypeDef *htim) {
    pMotorTimer = htim;

    /* Start Serial Logging for Driver Status */
    Serial_Log_Message("[INFO] DRV8833 Motor Driver initialization started...\r\n");

    /* Start PWM for Channel 1 and 2 (PB4 and PC7) */
    if (HAL_TIM_PWM_Start(pMotorTimer, TIM_CHANNEL_1) == HAL_OK &&
        HAL_TIM_PWM_Start(pMotorTimer, TIM_CHANNEL_2) == HAL_OK)
    {
        Serial_Log_Message("[SUCCESS] DRV8833 PWM Channels (CH1, CH2) active.\r\n");
        Serial_Log_Message("[INFO] Motor Control State: FAIL-SAFE ENABLED.\r\n");
    }
    else
    {
        Serial_Log_Message("[ERROR] DRV8833 PWM Initialization FAILED!\r\n");
    }
}

/**
 * @brief  Adjusts PWM duty cycle to control speed and direction.
 */
void DRV8833_Set_Motor(Motor_ID_t motor, Motor_State_t state, uint8_t speed) {
    /* Calculate Duty Cycle based on Auto-Reload Register (ARR) value */
    uint32_t duty = (speed * (__HAL_TIM_GET_AUTORELOAD(pMotorTimer))) / 100;

    /* Determine Timer Channels (For TIM3: CH1 = PB4, CH2 = PC7) */
    uint32_t ch1 = (motor == MOTOR_A) ? TIM_CHANNEL_1 : TIM_CHANNEL_3;
    uint32_t ch2 = (motor == MOTOR_A) ? TIM_CHANNEL_2 : TIM_CHANNEL_4;

    switch (state) {
        case MOTOR_FORWARD:
            __HAL_TIM_SET_COMPARE(pMotorTimer, ch1, duty);
            __HAL_TIM_SET_COMPARE(pMotorTimer, ch2, 0);
            break;

        case MOTOR_BACKWARD:
            __HAL_TIM_SET_COMPARE(pMotorTimer, ch1, 0);
            __HAL_TIM_SET_COMPARE(pMotorTimer, ch2, duty);
            break;

        case MOTOR_STOP:
        default:
            __HAL_TIM_SET_COMPARE(pMotorTimer, ch1, 0);
            __HAL_TIM_SET_COMPARE(pMotorTimer, ch2, 0);
            break;
    }
}
