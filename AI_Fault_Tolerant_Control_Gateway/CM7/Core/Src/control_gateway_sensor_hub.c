/************************************************************************
 * @file             : control_gateway_sensor_hub.c
 * @version          : v0.1.7 (Dynamic I/O Shift - Ultimate Baseline)
 * @author           : Mehmet Alperen Bakici
 * @date             : 2026.05.19
 * @brief            : Synchronous 1-Wire protocol driver for DHT11 on PD0.
 * Implements dynamic register-level directional I/O
 * switching to mitigate FreeRTOS context starvation.
 ************************************************************************/

#include "control_gateway_sensor_hub.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

/**
 * @brief  Compiler-hardened microsecond delay loop calibrated for a
 * 75 MHz APB/SYSCLK baseline configuration.
 * @param  us : Duration in microseconds.
 */
static void delay_us(uint32_t us)
{
    volatile uint32_t cycles = us * 7;
    while(cycles--)
    {
        __NOP();
    }
}

/**
 * @brief  Configures the PD0 pin mode to INPUT at register level
 * to eliminate physical driver contention.
 */
static void set_pin_as_input(void)
{
    GPIOD->MODER &= ~(GPIO_MODER_MODE0);
}

/**
 * @brief  Configures the PD0 pin mode to OUTPUT Push-Pull at register level
 * for high-drive active low/high transitions.
 */
static void set_pin_as_output(void)
{
    GPIOD->MODER &= ~(GPIO_MODER_MODE0);
    GPIOD->MODER |= (GPIO_MODE_OUTPUT_PP << GPIO_MODER_MODE0_Pos);
}

/**
 * @brief  Monitors line state changes with a hardware-hardened
 * timeout threshold to prevent task starvation.
 * @param  state           : Target GPIO state to wait for (SET/RESET).
 * @param  max_wait_loops  : Base scale multiplier for timeout execution.
 * @return uint8_t         : 1 if state transition occurs, 0 on hardware timeout.
 */
static uint8_t wait_for_pin_state(GPIO_PinState state, uint32_t max_wait_loops)
{
    volatile uint32_t timeout = max_wait_loops * 100;

    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == state)
    {
        if (--timeout == 0)
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief  Executes a safe, non-blocking 40-bit data acquisition sequence from DHT11.
 * Leverages dynamic directional switching between master and slave cycles.
 */
uint8_t DHT11_Read_Raw(float *pTemperature, float *pHumidity)
{
    uint8_t bits[5] = {0};
    volatile uint32_t bit_high_counter = 0;

    /* --- STEP 1: MASTER START SIGNAL (OUTPUT MODE) --- */
    set_pin_as_output();
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);
    osDelay(20);

    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    delay_us(10);

    /* --- STEP 2: TELEMETRY LISTENING (INPUT MODE) --- */
    set_pin_as_input();

    /* --- STEP 3: SENSOR HANDSHAKE VERIFICATION --- */
    /* Wait for sensor response low pulse (80us) */
    if (!wait_for_pin_state(GPIO_PIN_SET, 150)) return 0;

    /* Wait for sensor response high pulse (80us) */
    if (!wait_for_pin_state(GPIO_PIN_RESET, 150)) return 0;

    /* Wait for sensor trailing low edge prior to data stream initiation */
    if (!wait_for_pin_state(GPIO_PIN_SET, 150)) return 0;

    /* --- STEP 4: 40-BIT DATA STREAM PIPELINE DECODING --- */
    for (int j = 0; j < 40; j++)
    {
        /* Catch the baseline 50us low transmission guard */
        if (!wait_for_pin_state(GPIO_PIN_RESET, 150)) return 0;

        /* Measure active high pulse width to decode bit value */
        bit_high_counter = 0;
        while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET)
        {
            bit_high_counter++;
            delay_us(1);
            if (bit_high_counter > 200) return 0;
        }

        /* Calibrated timing threshold for 75 MHz: bit '0' (~26us) vs bit '1' (~70us) */
        if (bit_high_counter > 30)
        {
            bits[j / 8] |= (1 << (7 - (j % 8)));
        }
    }

    /* --- STEP 5: PAYLOAD CHECKSUM VALIDATION --- */
    if ((bits[0] + bits[1] + bits[2] + bits[3]) == bits[4])
    {
        *pHumidity    = (float)bits[0] + ((float)bits[1] * 0.1f);
        *pTemperature = (float)bits[2] + ((float)bits[3] * 0.1f);
        return 1;
    }

    return 0;
}
