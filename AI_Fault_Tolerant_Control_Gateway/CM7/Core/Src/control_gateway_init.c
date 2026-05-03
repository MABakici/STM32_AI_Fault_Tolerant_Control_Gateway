#include "control_gateway_init.h"
#include "control_gateway_lcd.h"

/* main.c içerisindeki handle'lara erişim */
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;


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

    Serial_Log_Message("[INFO] System startup sequence finalized.\r\n");

    /* LCD'yi baslat ve mesajı yazdir */
    LCD_Init();
}

