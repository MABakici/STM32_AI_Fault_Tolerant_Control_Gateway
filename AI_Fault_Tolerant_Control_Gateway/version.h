/*******************************************************************************
 *                      ISTANBUL TECHNICAL UNIVERSITY
 *                       Embedded Systems Laboratory
 *                                  2026
 *******************************************************************************

#ifndef __VERSION_H
#define __VERSION_H

/* Version Definitions */
#define PROJECT_NAME        "AI_FAULT_TOLERANT_CONTROL_GATEWAY"
#define SW_VERSION_MAJOR    0
#define SW_VERSION_MINOR    0
#define SW_VERSION_PATCH    6
#define BUILD_DATE          "2026-05-14"




/************************************************************************
* @version          : v0.0.6
* @author           : Mehmet Alperen Bakici
* @date             : 2026.05.14 02:20
* @branch           : main
*-------------------------------------------------------------------
* @notes            :
*
* - Integrated Fault-Tolerant Actuation:
* - Developed an automated safety-cutoff mechanism linking MPU6050 Tilt
*   detection directly to DRV8833 PWM generation.
* - Implemented "StartMotorCntTask" (Normal Priority) for real-time
*   motor state management and telemetric feedback.
*
* - Advanced UI & Display Architecture:
* - Engineered a 20x4 HD44780/PCF8574 Address Mapping logic to fix
*   multi-line cursor displacement (0x80, 0xC0, 0x94, 0xD4).
* - Applied "Constant Width Padding" (20-char normalization) to eliminate
*   screen flickering and ghosting without using global clearing commands.
*
* - Real-Time Performance Tuning:
* - Optimized IMU debounce filters from 500ms to 100ms for high-speed
*   safety response.
* - Synchronized Motor Control loop frequency to 50Hz for seamless
*   integration with IMU acquisition rates.
*
* - Hardware Abstraction:
* - Modularized DRV8833 driver logic using Timer PWM handles (TIM3)
*   with Common Ground (GND) synchronization for multi-power systems.
*
*************************************************************************/



/************************************************************************
* @version          : v0.0.5
* @author           : Mehmet Alperen Bakici
* @date             : 2026.05.11 02:15
* @branch           : main
*-------------------------------------------------------------------
* @notes            :
*
* - Asynchronous UI & State Management:
* - Engineered a non-blocking UI State Machine for real-time telemetry.
* - Implemented "Transient Message Locking" (3s TTL) for packet visualization.
* - Integrated System Uptime Telemetry on LCD Row 0 using FreeRTOS tasking.
* - Developed an auto-reset logic for the UI to resume monitoring mode.
*
* - Memory & Resource Optimization:
* - Applied "Strict Mutual Exclusion" logic to prevent LCD flickering/overlap.
* - Integrated UI timing states into centralized "Global_t" structure.
* - Implemented post-processing buffer sanitization for DMA security.
*
*************************************************************************/


/************************************************************************
* @version         : v0.0.4
* @author          : Mehmet Alperen Bakici
* @date            : 2026.05.10 12:10
* @branch          : main
*-------------------------------------------------------------------
* @notes           :
*
* - Communication Infrastructure & Protocol Design:
* - Implemented "StartCommunicationTask" for high-speed packet processing.
* - Developed a robust Binary Communication Protocol using 0x99AA Sync Headers.
* - Integrated UART DMA Circular Mode for USART3 to achieve zero-CPU overhead RX.
* - Implemented Checksum (CRC) validation for ensuring data frame integrity.
*
* - Event Dispatcher & Modular Architecture:
* - Established a Function Pointer based Event Dispatcher for efficient routing.
* - Decoupled communication logic from global state via "Comm_Handle_t" structure.
* - Applied Dependency Injection for modular and testable handler functions.
* - Created specialized handler prototypes for System, Motor, and AI control.
*
* - Documentation & Code Standards:
* - Standardized all function headers with official Doxygen-style technical notes.
* - Optimized workspace organization with dedicated .c/.h modules for comms.
*
*************************************************************************/


/************************************************************************
* @version         : v0.0.3
* @author          : Mehmet Alperen Bakici
* @date            : 2026.05.10 02:10
* @branch          : main
*-------------------------------------------------------------------
* @notes           :
*
* - Configured FreeRTOS task generation as "__weak" in CubeMX to prevent
*   "multiple definition" errors between main.c and freertos.c.
*
*************************************************************************/


/************************************************************************
* @version         : v0.0.2
* @author          : Mehmet Alperen Bakici
* @date            : 2026.05.10 01:32
* @branch          : main
*-------------------------------------------------------------------
* @notes           :
*
* - FreeRTOS (CMSIS_V2) Integration:
* - Implemented multi-tasking architecture for parallel processing.
* - Created StartIMUReadTask (High Priority) for 50Hz sensor polling.
* - Created StartAnalysisTask (Normal Priority) for real-time fault detection.
* - Created StartHeartbeatTask (Low Priority) for system health monitoring.
*
* - Sensor & Hardware Advancements:
* - Integrated MPU6050 IMU on dedicated I2C2 bus (PB10/PB11).
* - Implemented I2C Mutex protection to ensure bus arbitration safety.
* - Added Impact Detection (G-Force analysis) and Tilt Detection logic.
*
* - System Reliability:
* - Migrated HAL Timebase from SysTick to TIM1 for RTOS compatibility.
* - Configured Newlib Reentrancy for thread-safe math operations (sqrtf).
* - Stabilized inter-task communication via global shared memory structure.
*
*************************************************************************/


/************************************************************************
* @version         : v0.0.1
* @author          : Mehmet Alperen Bakici
* @date            : 2026.05.04 01:25
* @branch          : main
*-------------------------------------------------------------------
* @notes           :
*
*	 - First Commit.
*
*    - Initialized dual-core system architecture for STM32H745ZI.
*    - Configured .gitignore for workspace and build artifact optimization.
*    - Established UART3 (115200 8N1) high-speed logging system.
*    - Implemented I2C Bus Scanner for peripheral discovery.
*    - Integrated I2C LCD driver with H7 high-speed stability fixes.
*
*************************************************************************/


#endif /* __VERSION_H */
