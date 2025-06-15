/**
 * @file nvs_handler.h
 * @brief Declarations for Non-Volatile Storage (NVS) management functions.
 *
 * This header file provides the function prototypes for initializing NVS,
 * loading configuration settings from NVS, saving the current configuration
 * to NVS, and clearing the stored configuration. These functions are used
 * to persist device settings across reboots.
 */
#ifndef NVS_HANDLER_H
#define NVS_HANDLER_H

#include "config.h" 

/**
 * @brief Initializes the NVS (Non-Volatile Storage).
 * @return true if initialization is successful, false otherwise.
 */
bool initNVS();

/**
 * @brief Loads the configuration from NVS into global variables.
 * @return true if configuration was loaded successfully and the device mode
 * stored in NVS was MODE_CONFIGURED and a valid SSID was found.
 * Returns false otherwise.
 */
bool loadConfigurationFromNVS();

/**
 * @brief Saves the current configuration (from global variables) to NVS.
 * Sets the device mode to MODE_CONFIGURED in NVS.
 */
void saveConfigurationToNVS();

/**
 * @brief Clears the configuration in NVS, setting the mode to UNCONFIGURED.
 * Also clears sensitive configuration variables in RAM.
 */
void clearConfigurationInNVS();

#endif // NVS_HANDLER_H