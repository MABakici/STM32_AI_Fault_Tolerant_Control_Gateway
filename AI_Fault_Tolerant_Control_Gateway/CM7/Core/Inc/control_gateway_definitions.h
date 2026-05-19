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


/**
 * @brief  Hardened Sensor Fusion telemetry structure with explicit
 * memory alignment and DMA hardware shielding.
 * @note   Packed tightly to prevent compiler-inserted padding bytes.
 */
typedef struct __attribute__((packed)) {

    /* --- 32-Bit Aligned Variables (Floating Point Metrics) --- */
    float current_mA;           // Filtered brushed DC motor current transients in mA */
    float v_offset;             // Dynamically locked zero-current reference calibration in Volts */
    float temperature_C;        // Ambient environmental temperature in Celsius from DHT11 */
    float humidity_pct;         // Relative environmental humidity percentage from DHT11 */
    float light_intensity;      // Scaled ambient light intensity (Reserved for analog LDR) */

    /* --- DMA Hardware Isolation Zone --- */
    uint32_t memory_shield;     // Explicit memory wall isolating DMA bursts from float variables */
    volatile uint16_t adc_buffer[4]; /**< Hardware-level circular buffer for direct DMA ingestion */

    /* --- 8-Bit Diagnostics & Boolean Flags --- */
    uint8_t overcurrent_flag;   // Safety interlocking flag: 1 if current > 1500mA, else 0 */
    uint8_t is_dark;            // Optical contextual flag: 1 if environment is dark, 0 if well-lit */
    uint8_t sensor_status_bm;   // Bitmask representing telemetry health and peripheral status */

    uint8_t structural_padding; // Formal byte alignment to keep total struct size a multiple of 4 */

} Sensor_Fusion_t;

#endif
