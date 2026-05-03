/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @file     control_gateway_comm.c
 * @note     Implementation of communication protocols and bus management.
 ***********************************************************************************/

#include "control_gateway_comm.h"

/* External handles for peripherals */
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends a log message over the designated UART interface.
 * @param    msg: Pointer to the string to be transmitted.
 ***********************************************************************************/
void Serial_Log_Message(char *msg) {
    HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), 100);
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Scans the I2C bus to identify active peripheral addresses.
 ***********************************************************************************/
void I2C_Peripheral_Discovery(void) {
    Serial_Log_Message("\r\n[I2C] Scanning bus for active peripherals...\r\n");
    HAL_StatusTypeDef result;
    uint8_t device_count = 0;

    for (uint16_t i = 1; i < 128; i++) {
        /* Check device readiness by shifting address (7-bit to 8-bit) */
        result = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 10);

        if (result == HAL_OK) {
            char addr_msg[64];
            sprintf(addr_msg, "[I2C] Device detected at address: 0x%02X\r\n", i);
            Serial_Log_Message(addr_msg);
            device_count++;
        }
    }

    if (device_count == 0) {
        Serial_Log_Message("[I2C] CRITICAL: No peripherals detected on the bus!\r\n");
    } else {
        Serial_Log_Message("[I2C] Bus discovery completed successfully.\r\n");
    }
}
