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


typedef struct {
    /** DMA circular buffer. Sized to 4 elements to provide hardware-level
        buffer overflow containment against accidental 32-bit word writes. */
    volatile uint16_t adc_buffer[4];

    /** Hardware memory shield (padding variable). Formally forces a 32-bit
        alignment boundary to isolate DMA transactions from critical float variables. */
    uint32_t memory_shield;

    float current_mA;         // Suppressed and calibrated brushed DC motor current in mA
    float v_offset;           // Dynamically locked zero-current reference voltage in Volts
    float temperature_C;      // Ambient environmental temperature in Celsius
    float humidity_pct;       // Relative environmental humidity percentage
    float light_intensity;    // Ambient light intensity telemetry
    uint8_t overcurrent_flag; // Safety diagnostic flag: 1 if current exceeds 1500mA, else 0
    uint8_t sensor_status_bm; // Bitmask representing telemetry health and peripheral status
} Sensor_Fusion_t;
#endif
