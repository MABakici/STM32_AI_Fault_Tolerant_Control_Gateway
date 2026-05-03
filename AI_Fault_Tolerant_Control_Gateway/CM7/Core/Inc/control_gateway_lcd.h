/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @file     control_gateway_lcd.h
 * @note     Driver header for I2C LCD (PCF8574) integration.
 ***********************************************************************************/

#ifndef __CONTROL_GATEWAY_LCD_H
#define __CONTROL_GATEWAY_LCD_H

#include "main.h"

/* LCD I2C Address (Confirmed via discovery as 0x27) */
#define LCD_ADDR (0x27 << 1)

/* LCD Commands */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME   0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT  0x10
#define LCD_FUNCTIONSET  0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

/* Flags for display on/off control */
#define LCD_DISPLAYON 0x04
#define LCD_CURSOROFF 0x00
#define LCD_BLINKOFF  0x00

/* Backlight control */
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

/* Control Bits */
#define EN 0x04  // Enable bit
#define RS 0x01  // Register select bit

/* Function Prototypes */
/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Initializes the LCD over I2C in 4-bit mode.
 ***********************************************************************************/
void LCD_Init(void);

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends a specific command to the LCD.
 ***********************************************************************************/
void LCD_Send_Cmd(char cmd);

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends data (character) to the LCD.
 ***********************************************************************************/
void LCD_Send_Data(char data);

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sends a string to the current cursor position.
 ***********************************************************************************/
void LCD_Send_String(char *str);

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Sets cursor to specific row and column.
 ***********************************************************************************/
void LCD_Put_Cursor(int row, int col);

/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @note     Clears the entire display.
 ***********************************************************************************/
void LCD_Clear(void);

#endif /* __CONTROL_GATEWAY_LCD_H */
