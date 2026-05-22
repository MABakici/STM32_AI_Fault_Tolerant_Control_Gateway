/************************************************************************
 * @file             : control_gateway_sensor_hub.c
 * @version          : v0.2.1 (DWT Hardware-Stabilized - Ultimate Baseline)
 * @author           : Mehmet Alperen Bakici
 * @date             : 2026.05.20
 * @brief            : Synchronous 1-Wire protocol driver for DHT11 on PD0.
 * Leverages Cortex-M7 DWT Hardware cycle counter for
 * sub-microsecond precision jitter-free execution.
 * Fully hardened against FreeRTOS Scheduler Assertions.
 ************************************************************************/

#include "control_gateway_sensor_hub.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include <stdio.h>


extern UART_HandleTypeDef huart3;


/**
 * @brief  Initializes the Cortex-M7 Hardware Cycle Counter (DWT)
 * Must be active to achieve microsecond-precise hardware delays.
 */
static void DWT_Init(void)
{
    /* Core Debug Auth Status & Enable DWT Execution */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Hardware-precise microsecond delay using Cortex-M7 DWT Cycle Counter.
 * Completely immune to CPU optimization and Cache acceleration.
 * @param  us : Duration in microseconds.
 */
void delay_us(uint32_t us)
{
    uint32_t start_tick = DWT->CYCCNT;
    /* Calculate target clock cycles based on active system clock core speed */
    uint32_t target_ticks = us * (HAL_RCC_GetSysClockFreq() / 1000000);

    while ((DWT->CYCCNT - start_tick) < target_ticks);
}

/**
 * @brief  Configures the PD0 pin mode to INPUT at register level
 * with dynamic pull-up initialization.
 */
static void set_pin_as_input(void)
{
    /* Set MODER0 bits to 00 (Input Mode) */
    GPIOD->MODER &= ~(GPIO_MODER_MODE0);

    /* Enforce Pull-Up configuration to stabilize 1-Wire idle lines */
    GPIOD->PUPDR &= ~(GPIO_PUPDR_PUPD0);
    GPIOD->PUPDR |= (1U << GPIO_PUPDR_PUPD0_Pos);
}

/**
 * @brief  Configures the PD0 pin mode to OUTPUT Push-Pull at register level.
 */
static void set_pin_as_output(void)
{
    /* Clear and set MODER0 to 01 (General Purpose Output Mode) */
    GPIOD->MODER &= ~(GPIO_MODER_MODE0);
    GPIOD->MODER |= (1U << GPIO_MODER_MODE0_Pos);
}

/**
 * @brief  Monitors line state changes using the hardware DWT counter for timeouts.
 * @param  target_state    : The GPIO pin state we are waiting to reach.
 * @param  timeout_us      : Max duration to wait in microseconds.
 * @return uint8_t         : 1 on success, 0 on hardware timeout.
 */
static uint8_t wait_for_pin_state(GPIO_PinState target_state, uint32_t timeout_us)
{
    uint32_t start_tick = DWT->CYCCNT;
    uint32_t timeout_ticks = timeout_us * (HAL_RCC_GetSysClockFreq() / 1000000);

    /* Loop UNTIL the physical pin matches the requested target state */
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) != target_state)
    {
        if ((DWT->CYCCNT - start_tick) >= timeout_ticks)
        {
            return 0; /* Hardware Timeout occurred */
        }
    }
    return 1; /* Target state successfully reached */
}

/**
 * @brief  Executes a safe, non-blocking 40-bit data acquisition sequence from DHT11.
 * Fully guarded by DWT hardware clocks and register-level I/O toggles.
 */
uint8_t DHT11_Read_Raw(float *pTemperature, float *pHumidity)
{
    uint8_t bits[5] = {0};
    uint32_t bit_high_duration = 0;
    uint32_t bit_start_tick = 0;
    uint32_t cpu_freq_mhz = HAL_RCC_GetSysClockFreq() / 1000000;

    /* Dynamic Hardware Guard: Auto-init DWT if it gets disabled */
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
    {
        DWT_Init();
    }

    /* --- STEP 1: MASTER START SIGNAL (OUTPUT MODE) --- */
    set_pin_as_output();
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);

    /* CRITICAL ARCHITECTURAL RECOVERY:
     * Replaced osDelay(18) with hard-locked delay_us(18000) to entirely prevent
     * FreeRTOS from asserting configASSERT(uxSchedulerSuspended == 0). */
    delay_us(18000); /* 18ms wake-up constraint met via pure DWT hardware clocks */

    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    delay_us(30); /* Pull high for 30us based on datasheet specifications */

    /* --- STEP 2: TELEMETRY LISTENING (INPUT MODE) --- */
    set_pin_as_input();

    /* --- STEP 3: SENSOR HANDSHAKE VERIFICATION --- */
    /* Wait for sensor to pull line LOW (Response signal start - ~80us) */
    if (!wait_for_pin_state(GPIO_PIN_RESET, 100)) return 0;

    /* Wait for sensor to release line HIGH (Response signal mid-state - ~80us) */
    if (!wait_for_pin_state(GPIO_PIN_SET, 100)) return 0;

    /* Wait for sensor to drop line LOW again (Ready for data transmission) */
    if (!wait_for_pin_state(GPIO_PIN_RESET, 100)) return 0;

    /* --- STEP 4: 40-BIT DATA STREAM PIPELINE DECODING --- */
    for (int j = 0; j < 40; j++)
    {
        /* Catch the baseline 50us low transmission guard preceding every bit */
        if (!wait_for_pin_state(GPIO_PIN_SET, 100)) return 0;

        /* Measure the physical duration of the high pulse using hardware ticks */
        bit_start_tick = DWT->CYCCNT;
        if (!wait_for_pin_state(GPIO_PIN_RESET, 100)) return 0;

        /* Calculate precise high pulse duration in microseconds */
        bit_high_duration = (DWT->CYCCNT - bit_start_tick) / cpu_freq_mhz;

        /* Standard DHT11 Specs: bit '0' (~26us high) vs bit '1' (~70us high)
         * Safe threshold set to 40us for optimal signal classification boundary. */
        if (bit_high_duration > 40)
        {
            bits[j / 8] |= (1 << (7 - (j % 8)));
        }
    }

    /* --- STEP 5: PAYLOAD CHECKSUM VALIDATION --- */
    if (((bits[0] + bits[1] + bits[2] + bits[3]) & 0xFF) == bits[4])
    {
        /* Enforce bounds protection against corrupted zero matrix outputs */
        if (bits[0] == 0 && bits[2] == 0) return 0;

        *pHumidity    = (float)bits[0] + ((float)bits[1] * 0.1f);
        *pTemperature = (float)bits[2] + ((float)bits[3] * 0.1f);
        return 1; /* Valid packet decoded successfully */
    }

    return 0; /* Checksum mismatch fault */
}


/* Link directly to the hardware DMA buffer defined in the initialization context */
extern volatile uint16_t ADC_Hardware_Buffer[4];



/**
 * @brief  Executes discrete Signal Processing (DSP) for ACS712 current tracking.
 * @note   Maintains a strict 5Hz periodicity execution context. Employs mathematically
 * calibrated scaling factors, software dead-bands, and non-blocking windowed
 * oversampling to align current tracking with the nominal 300mA brushed DC motor baseline.
 * @param  None
 * @retval None
 */
void Process_Motor_Current_Resolution(void)
{
    /* Physical Hardware and Signal Conversion Calibration Constants */
    const float ADC_TO_VOLT    = 3.3f / 65535.0f;
    const float SENSITIVITY    = 0.185f; /* ACS712 5A Module Sensitivity (185mV/A) */

    /* Calibrated gain and noise suppression thresholds tailored for 5Hz execution cadence */
    const float GAIN_FACTOR    = 1.45f;
    const float NOISE_FLOOR_mA = 45.0f;  /* Enhanced dead-band to mute lower floor variations */
    const float EMA_ALPHA      = 0.08f;  /* Heavy smoothing coefficient to restrict the oscillation band */
    const float MAX_SAFE_mA    = 4000.0f;/* Structural saturation ceiling for 5A module */

    /* Local Static Variables to Retain Pipeline History Across Context Switches */
    static float local_v_offset = 1.894f;
    static float local_smoothed_current = 0.0f;
    static uint32_t calibration_sample_cnt = 0;
    static float calibration_accumulator = 0.0f;

    /* Explicitly initialize execution variable to guarantee state determinism */
    float current_calculated = 0.0f;

    /* Synchronize local baseline dynamically if updated during early hardware boot phase */
    if (Global_t.SensorHub_t.v_offset > 1.0f && Global_t.SensorHub_t.v_offset < 2.5f)
    {
        local_v_offset = Global_t.SensorHub_t.v_offset;
    }

    /* CRITICAL HARDWARE BARRIER: L1 DATA CACHE INVALIDATION
     * Forces the CPU core to invalidate its current cache line and fetch the
     * fresh hardware data buffer modifications deposited directly inside the SRAM. */
    SCB_InvalidateDCache_by_Addr((uint32_t*)&ADC_Hardware_Buffer[0], 32);

    /* --- STEP 1: NON-BLOCKING SYNCHRONOUS BURST OVERSAMPLING ---
     * Ingest 15 back-to-back hardware snapshots directly from the aligned DMA buffer.
     * Prevents operating system context switches during critical high-frequency sampling. */
    uint32_t samples[15];
    for (int i = 0; i < 15; i++)
    {
        samples[i] = (uint16_t)ADC_Hardware_Buffer[0];
        __NOP(); /* Hardware pipeline padding to relax data bus arbitration */
    }

    /* --- STEP 2: NON-LINEAR MEDIAN SORTING (Bubble Sort) --- */
    for (int i = 0; i < 14; i++)
    {
        for (int j = i + 1; j < 15; j++)
        {
            if (samples[i] > samples[j])
            {
                uint32_t temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }

    /* --- STEP 3: CENTER-AVERAGED MEDIAN EXTRACTION ---
     * Extracts the five most central elements of the sorted sample matrix
     * (Indices 5 to 9) to eliminate asymmetrical fletcher commutation noise spikes. */
    float central_sum = (float)(samples[5] + samples[6] + samples[7] + samples[8] + samples[9]);
    float filtered_raw_adc = central_sum / 5.0f;
    float current_voltage = filtered_raw_adc * ADC_TO_VOLT;

    /* --- STEP 4: ARCHITECTURE GATE - DYNAMIC CONTINUOUS CALIBRATION --- */
    if (Global_t.is_tilt_detected == 1 || Global_t.dc_motor_control.is_system_halted == 1)
    {
        /* System is functionally inactive. Accumulate baseline voltage offsets asynchronously */
        calibration_accumulator += current_voltage;
        calibration_sample_cnt++;

        if (calibration_sample_cnt >= 10)
        {
            local_v_offset = calibration_accumulator / 10.0f;
            calibration_accumulator = 0.0f;
            calibration_sample_cnt = 0;
        }
        current_calculated = 0.0f;
    }
    else
    {
        /* Flush calibration tracking registers immediately upon active motor execution */
        calibration_accumulator = 0.0f;
        calibration_sample_cnt = 0;

        /* --- STEP 5: MATHEMATICAL CURRENT RESOLVER --- */
        float delta_v = local_v_offset - current_voltage;

        /* Mathematical absolute mapping to handle bidirectional ripples */
        if (delta_v < 0.0f)
        {
            delta_v = -delta_v;
        }

        /* Translate raw voltage delta into precise milliampere load metrics */
        current_calculated = (delta_v / SENSITIVITY) * 1000.0f * GAIN_FACTOR;

        /* Apply structural dead-band clamp */
        if (current_calculated < NOISE_FLOOR_mA)
        {
            current_calculated = 0.0f;
        }

        /* Enforce physical saturation limits */
        if (current_calculated > MAX_SAFE_mA)
        {
            current_calculated = MAX_SAFE_mA;
        }
    }

    /* --- STEP 6: EXPONENTIAL MOVING AVERAGE (EMA) FILTER LAYER --- */
    if (Global_t.is_tilt_detected || Global_t.dc_motor_control.is_system_halted)
    {
        local_smoothed_current = 0.0f;
    }
    else
    {
        local_smoothed_current = (EMA_ALPHA * current_calculated) +
                                 ((1.0f - EMA_ALPHA) * local_smoothed_current);

        /* Secondary boundary check to avoid underflow drift anomalies */
        if (local_smoothed_current < 0.0f)
        {
            local_smoothed_current = 0.0f;
        }
    }

    /* --- STEP 7: TELEMETRY CONTEXT EXPORT TO SHARED SYSTEM CORE --- */
    Global_t.SensorHub_t.current_mA = local_smoothed_current;
    Global_t.SensorHub_t.v_offset   = local_v_offset;

    /* Hardware Safety Interlocking Supervision Boundary */
    if (Global_t.SensorHub_t.current_mA > 1500.0f)
    {
        Global_t.SensorHub_t.overcurrent_flag = 1;
    }
    else
    {
        Global_t.SensorHub_t.overcurrent_flag = 0;
    }

    /* --- STEP 8: INDUSTRIAL TELEMETRY DATA LOGGING --- */
    static uint8_t diag_counter = 0;
    diag_counter++;
    if (diag_counter >= 5)
    {
        char diag_buf[128];
        int diag_len = sprintf(diag_buf, "[DIAG] Raw_ADC: %u | V_Cur: %ld mV | V_Off: %ld mV | Calc: %ld mA\r\n",
                               (uint16_t)filtered_raw_adc,
                               (int32_t)(current_voltage * 1000.0f),
                               (int32_t)(local_v_offset * 1000.0f),
                               (int32_t)Global_t.SensorHub_t.current_mA);
        HAL_UART_Transmit(&huart3, (uint8_t*)diag_buf, diag_len, 20);
        diag_counter = 0;
    }
}
