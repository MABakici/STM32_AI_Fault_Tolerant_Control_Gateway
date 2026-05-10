#include "control_gateway_comm_handler.h"

/* External function prototypes defined in specialized application modules */

/**
 * @brief  Handler for system-wide initialization and reset commands.
 * @param  val: Command parameter for specific initialization type.
 */
extern void Execute_System_Init(uint8_t val);

/**
 * @brief  Handler for DC motor speed regulation via PWM duty cycle adjustment.
 * @param  val: Target duty cycle percentage (0-100).
 */
extern void Execute_Motor_Control(uint8_t val);

/**
 * @brief  Handler for tuning AI anomaly detection sensitivity thresholds.
 * @param  val: Sensitivity scale factor.
 */
extern void Execute_AI_Config(uint8_t val);


/**********************************************************************************
 * @author    Mehmet Alperen BAKICI
 * @date      10.05.2026
 * @note      Dispatches validated commands to their respective specialized handlers.
 ***********************************************************************************/
void Comm_Dispatch_Event(uint8_t cmd, uint8_t data)
{
    /* Array of registered command handlers mapping IDs to functions */
    static const Dispatcher_Entry_t dispatcher_table[] = {
        {CMD_SYS_INIT,   Execute_System_Init,   "System Initialization"},
        {CMD_MOTOR_CTRL, Execute_Motor_Control, "DC Motor Regulation"  },
        {CMD_AI_CONFIG,  Execute_AI_Config,    "AI Threshold Tuning"  }
    };

    const uint8_t table_size = (uint8_t)(sizeof(dispatcher_table) / sizeof(Dispatcher_Entry_t));

    for (uint8_t i = 0; i < table_size; i++)
    {
        if (dispatcher_table[i].cmd_id == cmd)
        {
            /* Execution of the associated callback function */
            dispatcher_table[i].handler(data);
            return;
        }
    }
}

/**
 * @brief  Protocol Analysis Engine: Validates frames and manages transient UI states.
 * @author Mehmet Alperen BAKICI
 * @date   11.05.2026
 * @param  hComm: Pointer to the communication handle.
 * @note   Updated to lock UI updates for 3 seconds upon valid packet arrival.
 */
void Comm_Process_Packet(Comm_Handle_t *hComm)
{
    /* Null pointer protection for robust execution */
    if ( ( hComm == NULL ) || ( hComm->pRxBuffer == NULL ) )
    {
        return;
    }

    /* Map local pointers to the incoming raw data stream */
    uint8_t *raw = hComm->pRxBuffer;
    uint8_t cmd  = 0;
    uint8_t data = 0;
    uint8_t crc  = 0;

    /* SECTION 1: Frame Synchronization (Header: 0x99 0xAA) */
    if ( ( raw[0] == FRAME_HEADER_1 ) && ( raw[1] == FRAME_HEADER_2 ) )
    {
        cmd  = raw[2];
        data = raw[3];
        crc  = raw[4];

        /* SECTION 2: Protocol Integrity Validation (CMD + DATA == CRC) */
        if ( (uint8_t)( cmd + data ) == crc )
        {
            /* Trigger UI Lock: Signals StartCommTask to suspend uptime updates for 3 seconds */
            Global_t.uart_gateway.last_packet_tick     = HAL_GetTick();
            Global_t.uart_gateway.display_timeout_flag = 1;

            char line1[20];
            char line2[20];

            memset(line1, 0, sizeof(line1));
            memset(line2, 0, sizeof(line2));

            /* SECTION 3: Transient UI Data Formatting */
            /* Clear line with trailing spaces to prevent character overlap */
            sprintf(line1, "RECV ID:%02X      ", cmd);

            /* Map command IDs to human-readable functional descriptors */
            switch(cmd)
            {
                case 0x01: sprintf(line2, "VAL:%-3d SYS INIT ", data); break;
                case 0x02: sprintf(line2, "VAL:%-3d MOTOR CTR", data); break;
                case 0x03: sprintf(line2, "VAL:%-3d AI CONFIG", data); break;
                default:   sprintf(line2, "VAL:%-3d UNKNOWN   ", data); break;
            }

            /* SECTION 4: Immediate Physical UI Update */
            /* This content will be held for 3 seconds by the task scheduler */
            LCD_Clear();
            LCD_Put_Cursor(0, 0);
            LCD_Send_String(line1);

            LCD_Put_Cursor(1, 0);
            LCD_Send_String(line2);

            /* SECTION 5: Serial Telemetry (CLI Feedback) */
            printf("\r\n[GATEWAY] Valid Packet Processed.\r\n");
            printf("[GATEWAY] Action: %s | Data: %d\r\n", line2 + 8, data);

            /* Trigger the specialized event dispatcher for command execution */
            Comm_Dispatch_Event(cmd, data);
        }
        else
        {
            /* Checksum validation failure - Update telemetry metrics */
            printf("[ERROR] Checksum Mismatch detected for ID: 0x%02X\r\n", cmd);

            if ( hComm->pErrorCounter != NULL )
            {
                ( *( hComm->pErrorCounter ) )++;
            }
        }
    }

    /* SECTION 6: Buffer Sanitization */
    /* Flush the RX buffer to prevent stale data re-processing */
    memset(hComm->pRxBuffer, 0, 5);
}
