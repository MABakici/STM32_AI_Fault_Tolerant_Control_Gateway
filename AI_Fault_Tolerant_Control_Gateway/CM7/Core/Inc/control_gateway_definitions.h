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



#endif
