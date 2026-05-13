/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @file     control_gateway_lcd.c
 * @note     Implementation of I2C LCD (PCF8574) driver functions.
 ***********************************************************************************/

#include "control_gateway_lcd.h"
#include "control_gateway_comm.h" // Serial_Log_Message kullanmak icin

extern I2C_HandleTypeDef hi2c1;

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Internal helper function to send 4-bit data to LCD.
 ***********************************************************************************/
static void LCD_Send_Internal(char data, int rs_bit)
{
	uint8_t data_u, data_l;
	uint8_t pkt;

	data_u = (data & 0xf0);
	data_l = ((data << 4) & 0xf0);

	// --- Upper Nibble ---
	pkt = data_u | rs_bit | EN | LCD_BACKLIGHT;
	HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &pkt, 1, 10);
	for(volatile int i=0; i<10000; i++); // Enable pulse delay

	pkt = data_u | rs_bit | LCD_BACKLIGHT;
	HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &pkt, 1, 10);
	for(volatile int i=0; i<10000; i++); // Wait for LCD to process

	// --- Lower Nibble ---
	pkt = data_l | rs_bit | EN | LCD_BACKLIGHT;
	HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &pkt, 1, 10);
	for(volatile int i=0; i<10000; i++);

	pkt = data_l | rs_bit | LCD_BACKLIGHT;
	HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &pkt, 1, 10);
	for(volatile int i=0; i<10000; i++);
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends a command to the LCD.
 ***********************************************************************************/
void LCD_Send_Cmd(char cmd) {
    LCD_Send_Internal(cmd, 0);
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends data/character to the LCD.
 ***********************************************************************************/
void LCD_Send_Data(char data) {
    LCD_Send_Internal(data, RS);
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Initializes the LCD in 4-bit mode and displays startup message.
 ***********************************************************************************/
void LCD_Init(void)
{
    // Güç açıldıktan sonra ekranın kendine gelmesi için bekleme
    HAL_Delay(100);

    // 4-bit moduna zorla geçiş sekansı
    LCD_Send_Cmd(0x33); // Önce 8-bit gibi gönder
    HAL_Delay(5);
    LCD_Send_Cmd(0x32); // Sonra 4-bit moduna hazırla
    HAL_Delay(5);
    LCD_Send_Cmd(0x28); // 2 Satır, 5x8 Font
    HAL_Delay(5);

    // Ekran ayarları
    LCD_Send_Cmd(0x0C); // Display ON, Cursor OFF
    HAL_Delay(5);
    LCD_Send_Cmd(0x06); // Entry mode: sağa doğru yaz
    HAL_Delay(5);

    LCD_Clear();
    HAL_Delay(10);

    // Test Mesajı
    LCD_Put_Cursor(0, 0);
    LCD_Send_String("GATEWAY SYSTEM");
    LCD_Put_Cursor(1, 0);
    LCD_Send_String("STABLE NOW!");

    Serial_Log_Message("[INFO] LCD Refreshed with stable timings.\r\n");
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends a string of characters to the LCD.
 ***********************************************************************************/
void LCD_Send_String(char *str) {
    while (*str) LCD_Send_Data(*str++);
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     14.05.2026 (Updated for 20x4 LCD)
 * @note     Positions the cursor on the screen (0-3 for rows).
 ***********************************************************************************/
void LCD_Put_Cursor(int row, int col) {
    uint8_t address;
    switch (row) {
        case 0:
            address = 0x80 + col; // 1. Satır başı
            break;
        case 1:
            address = 0xC0 + col; // 2. Satır başı
            break;
        case 2:
            address = 0x94 + col; // 3. Satır başı (20x4 LCD özel adresi)
            break;
        case 3:
            address = 0xD4 + col; // 4. Satır başı (20x4 LCD özel adresi)
            break;
        default:
            address = 0x80; // Hatalı satır gelirse en başa dön
            break;
    }
    LCD_Send_Cmd(address);
}

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Clears the screen.
 ***********************************************************************************/
void LCD_Clear(void) {
    LCD_Send_Cmd(LCD_CLEARDISPLAY);
    HAL_Delay(2);
}
