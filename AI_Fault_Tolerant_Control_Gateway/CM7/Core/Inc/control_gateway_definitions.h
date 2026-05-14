#ifndef CONTROL_GATEWAY_DEFINITIONS_H_
#define CONTROL_GATEWAY_DEFINITIONS_H_

#include "stdint.h"


typedef struct {
    uint8_t   rx_buffer    [16];    // Raw DMA buffer
    uint8_t   main_buffer  [64];    // Processed data buffer
    uint8_t   packet_ready     ;    // Flag for CommunicationTask
    uint16_t  rx_index         ;
    uint32_t  uart_error_cnt   ;

    uint32_t last_packet_tick;     // Paketin geldiği an (ms)
    uint8_t  display_timeout_flag; // Ekran temizleme bekliyor mu?
} UART_Comm_t;

typedef struct
{
    uint8_t  target_speed;      // Target speed percentage (0-100)
    uint8_t  is_system_halted;  // Software emergency stop flag (0: Run, 1: Halt)
    uint16_t current_rpm;       // Placeholder for future encoder/estimation data
    uint8_t  active_direction;  // 0: Forward, 1: Backward
} Motor_Control_t;

#endif
