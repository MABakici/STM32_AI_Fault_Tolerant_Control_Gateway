#ifndef CONTROL_GATEWAY_DEFINITIONS_COMM_HANDLER_H_
#define CONTROL_GATEWAY_DEFINITIONS_COMM_HANDLER_H_

#include "main.h"
#include "control_gateway_lcd.h"
#include <stdio.h>  /* Required for sprintf */
#include "global_variables_CM7.h"

/* Frame Definitions */
#define FRAME_HEADER_1    ( 0x99 )
#define FRAME_HEADER_2    ( 0xAA )
#define FRAME_LENGTH      ( 5    )

/* Command ID Definitions */
#define CMD_SYS_INIT      ( 0x01 )
#define CMD_MOTOR_CTRL    ( 0x02 )
#define CMD_AI_CONFIG     ( 0x03 )

/**
 * @brief Communication Handle Structure
 * Used for dependency injection to decouple from global variables.
 */
typedef struct {
    uint8_t  *pRxBuffer     ;  /* Pointer to raw DMA buffer             */
    uint32_t *pErrorCounter ;  /* Pointer to UART error tracking metric */
} Comm_Handle_t;

typedef void (*Handler_Func_t)(uint8_t val);

typedef struct {
    uint8_t          cmd_id      ;
    Handler_Func_t   handler     ;
    const char      *description ;
} Dispatcher_Entry_t;

/* Function Prototypes - Parameters injected from caller */
void Comm_Process_Packet(Comm_Handle_t *hComm);
void Comm_Dispatch_Event(uint8_t cmd, uint8_t data);

#endif /* CONTROL_GATEWAY_DEFINITIONS_COMM_HANDLER_H_ */
