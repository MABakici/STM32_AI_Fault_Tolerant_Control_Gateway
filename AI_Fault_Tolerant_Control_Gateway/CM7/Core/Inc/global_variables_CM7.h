#ifndef GLOBAL_VARIABLES_CM7_H_
#define GLOBAL_VARIABLES_CM7_H_

#include <control_gateway_definitions.h>
#include "MPU6050.h"
#include <stdint.h>


typedef struct
{

	MPU6050_t       mpu6050_veriler       ;
    uint8_t         is_tilt_detected      ;
    uint8_t         is_impact_detected    ;
    uint32_t        i2c_error_cnt         ;

    uint32_t        system_heartbeat      ;

    UART_Comm_t uart_gateway;   // UART Communication handler

} Global_Variables_t;


extern Global_Variables_t Global_t;

#endif
