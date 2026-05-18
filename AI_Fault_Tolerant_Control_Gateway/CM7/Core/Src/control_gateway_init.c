#include <control_gateway_drv8833_driver.h>
#include <global_variables_CM7.h>
#include <mpu6050.h>
#include "control_gateway_init.h"
#include "control_gateway_lcd.h"


/* main.c içerisindeki handle'lara erişim */
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c2;
extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;

/**********************************************************************************
 * @author    Mehmet Alperen BAKICI
 * @date      03.05.2026
 * @note      Control Gateway system initialization and peripheral management.
 * @warning
 ***********************************************************************************/
void AI_Based_Fault_Tolerant_Control_Gateway_Init(void)
{
    /* Donanim stabilizasyonu icin kisa bir bekleme */
    HAL_Delay(150);

    Serial_Log_Message("\r\n==========================================\r\n");
    Serial_Log_Message("  STM32 AI FAULT TOLERANT CONTROL GATEWAY \r\n");
    Serial_Log_Message("       Cortex-M7 Application Started      \r\n");
    Serial_Log_Message("==========================================\r\n");

    Serial_Log_Message("[INFO] Peripherals initialization sequence started...\r\n");

    /* I2C hattindaki cihazlari (LCD, MPU6050 vb.) tespit et */
    I2C_Peripheral_Discovery();

    /* MPU6050 Init */
    MPU6050_Gateway_Init(&hi2c2, &Global_t.mpu6050_veriler);

    /* LCD'yi baslat ve mesajı yazdir */
    LCD_Init();

    /* Motor driver baslatiliyor. */
    DRV8833_Init(&htim3);

    /* Enable UART IDLE Interrupt */
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

    /* ADC1'i DMA modunda başlatıp veriyi direkt SensorHub_t yapısının içine akıtıyoruz */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)Global_t.SensorHub_t.adc_buffer, 1);

    /* ACS712 Calibration Task */
    ACS712_PowerOn_Calibration();

    Serial_Log_Message("[INFO] System startup sequence finalized.\r\n");
}


/**********************************************************************************
 * @author    Mehmet Alperen BAKICI
 * @date      18.05.2026
 * @note      Performs a synchronous 50-sample power-on self calibration for ACS712.
 *            Must be invoked after HAL_ADC_Start_DMA and BEFORE starting FreeRTOS scheduler.
 *            Modifies the global sensor hub v_offset baseline securely.
 * @warning
 ***********************************************************************************/
void ACS712_PowerOn_Calibration(void)
{
    const float ADC_TO_VOLT = 3.3f / 65535.0f;
    float startup_offset_sum = 0.0f;
    char log_buf[128];

    /* Global shared variable initialization */
    Global_t.SensorHub_t.current_mA = 0.0f;

    /* Notify host that calibration has initiated */
    Serial_Log_Message("[INFO] ACS712 Current Sensor calibration initiated...\r\n");

    /* Collect 50 stable raw samples while motor is guaranteed to be at rest */
    for(int c = 0; c < 50; c++)
    {
        startup_offset_sum += (float)((uint16_t)Global_t.SensorHub_t.adc_buffer[0]) * ADC_TO_VOLT;
        HAL_Delay(10); /* Use HAL_Delay since FreeRTOS scheduler is not yet ticking */
    }

    /* Commit calculated baseline to global shared memory */
    Global_t.SensorHub_t.v_offset = startup_offset_sum / 50.0f;

    /* Synchronous logging: Print the exact calibrated reference voltage in mV
       Format matches industrial diagnostic boot screens */
    int log_len = sprintf(log_buf, "[SUCCESS] ACS712 Current Sensor Calibrated. Reference Voltage: %ld mV\r\n",
                          (int32_t)(Global_t.SensorHub_t.v_offset * 1000.0f));

    /* Direct hardware transmit to bypass printf float overhead */
    HAL_UART_Transmit(&huart3, (uint8_t*)log_buf, log_len, 50);
}
