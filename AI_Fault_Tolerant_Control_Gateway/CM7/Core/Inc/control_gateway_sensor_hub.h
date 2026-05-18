/************************************************************************
 * @file             : control_gateway_sensor_hub.h
 * @version          : v0.1.7 (Dynamic I/O Shift - Ultimate Baseline)
 * @author           : Mehmet Alperen Bakici
 * @date             : 2026.05.19
 * @brief            : Sensor Hub driver header for environmental telemetry.
 ************************************************************************/

#ifndef INC_CONTROL_GATEWAY_SENSOR_HUB_H_
#define INC_CONTROL_GATEWAY_SENSOR_HUB_H_

#include "main.h"

/**
 * @brief  Executes a safe, non-blocking 40-bit data acquisition sequence from DHT11.
 * @param  pTemperature : Pointer to store the decoded Celsius temperature value.
 * @param  pHumidity    : Pointer to store the decoded relative humidity percentage.
 * @return uint8_t      : 1 if checksum matches and payload is valid, 0 otherwise.
 */
uint8_t DHT11_Read_Raw(float *pTemperature, float *pHumidity);

#endif /* INC_CONTROL_GATEWAY_SENSOR_HUB_H_ */
