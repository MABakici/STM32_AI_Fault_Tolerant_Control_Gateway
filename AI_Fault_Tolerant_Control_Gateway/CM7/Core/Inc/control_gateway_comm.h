/**********************************************************************************
 * @author   Mehmet Alperen BAKICI
 * @date     03.05.2026
 * @file     control_gateway_comm.h
 * @note     Communication layer interface for I2C, UART and peripheral discovery.
 ***********************************************************************************/

#ifndef __CONTROL_GATEWAY_COMM_H
#define __CONTROL_GATEWAY_COMM_H

#include "main.h"
#include <string.h>
#include <stdio.h>

/* Function Prototypes */
void Serial_Log_Message(char *msg);
void I2C_Peripheral_Discovery(void);

#endif /* __CONTROL_GATEWAY_COMM_H */
