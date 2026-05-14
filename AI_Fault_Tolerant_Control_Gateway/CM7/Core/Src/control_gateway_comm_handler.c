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
 * @brief  Protocol Analysis Engine: Validates frames and triggers dispatch events.
 * @author Mehmet Alperen BAKICI
 * @date   14.05.2026
 * @note   UI updates are handled asynchronously by StartCommTask via display_timeout_flag.
 */
void Comm_Process_Packet(Comm_Handle_t *hComm)
{
    if ((hComm == NULL) || (hComm->pRxBuffer == NULL)) return;

    uint8_t *raw = hComm->pRxBuffer;

    /* SECTION 1: Frame Synchronization (Header: 0x99 0xAA) */
    if ((raw[0] == FRAME_HEADER_1) && (raw[1] == FRAME_HEADER_2))
    {
        uint8_t cmd  = raw[2];
        uint8_t data = raw[3];
        uint8_t crc  = raw[4];

        /* SECTION 2: Protocol Integrity Validation */
        if ((uint8_t)(cmd + data) == crc)
        {
            /**
             * @section Visual State Locking
             * Mühürleme burada yapılır. StartCommTask uyandığında bu bayrağı
             * görüp 3 saniye boyunca ekranı kilitleyecektir.
             */
            Global_t.uart_gateway.last_packet_tick     = HAL_GetTick();
            Global_t.uart_gateway.display_timeout_flag = 1;

            /* SECTION 3: Dispatch & Execution */
            Comm_Dispatch_Event(cmd, data);

            /* SECTION 4: CLI Telemetry */
            printf("[GATEWAY] Valid Packet Processed. ID: 0x%02X\r\n", cmd);
        }
        else
        {
            if (hComm->pErrorCounter != NULL) (*(hComm->pErrorCounter))++;
        }
    }

    /* SECTION 5: Buffer Sanitization */
    memset(hComm->pRxBuffer, 0, 5);
}
