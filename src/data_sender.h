/**
 * @file data_sender.h
 * @brief Function declarations for sensor initialization, data processing, and network communication tasks.
 *
 * This header file declares functions related to initializing the BME280 sensor,
 * calculating meteorological values like Mean Sea Level pressure, sending device
 * registration information, and the main FreeRTOS tasks for collecting and
 * transmitting sensor data (environmental and wind).
 */
#ifndef DATA_SENDER_H
#define DATA_SENDER_H

#include "config.h"

/**
 * @brief Initializes the BME280 sensor object.
 * @note In this version, it might contain Wire.begin() and be called repeatedly.
 * @return true if initialization was successful, false otherwise.
 */
bool initBME280();

/**
 * @brief Reduces station pressure to Mean Sea Level (MSL) pressure.
 * @param station_pressure_hpa Measured pressure at the station in hectopascals (hPa).
 * @param station_temperature_c Measured temperature at the station in Celsius (°C).
 * @param station_altitude_m Altitude of the station above sea level in meters (m).
 * @return Pressure reduced to MSL in hPa, or NAN if input data is invalid.
 */
double reduceToMSL(double station_pressure_hpa, double station_temperature_c, double station_altitude_m);

/**
 * @brief Sends the device's MAC address to the API registration endpoint.
 * @note Works only in STA mode and when WiFi is connected.
 */
void sendMacAddress();

/**
 * @brief FreeRTOS task function to periodically read environmental sensor data
 * (BME280, photoresistor, rain), create a JSON payload, and send it to the API data endpoint.
 * Handles HTTP errors and potentially updates LED status.
 * @param pvParameters Standard FreeRTOS task parameters (unused).
 */
void sensorTaskFunction(void *pvParameters); // <<< RENAMED/CHANGED

/**
 * @brief FreeRTOS task function to periodically read wind sensor data.
 * This task typically reads the wind sensor at a higher frequency and makes
 * aggregated data available for the main sensorTaskFunction.
 * @param pvParameters Standard FreeRTOS task parameters (unused).
 */
void windSensorTaskFunction(void *pvParameters);

#endif // DATA_SENDER_H