/**
 * @file nvs_handler.cpp
 * @brief Manages Non-Volatile Storage (NVS) operations for the ESP32.
 *
 * This file provides functions to initialize NVS, load configuration settings
 * (like Wi-Fi credentials, server address, username, and device mode) from NVS
 * into global variables, save the current configuration to NVS, and clear
 * the stored configuration, effectively resetting the device to an unconfigured state.
 */
#include "nvs_handler.h"
#include "config.h"      
#include <Preferences.h> 

// --- Global Objects ---
Preferences preferences; // Global Preferences object for NVS access

// --- NVS Initialization ---

/**
 * @brief Initializes the NVS (Non-Volatile Storage).
 * Checks if the namespace exists and creates it if not.
 * Ensures the device mode key exists, setting it to UNCONFIGURED if absent.
 * @return true if initialization is successful, false otherwise.
 */
bool initNVS() {    
    if (!preferences.begin(NVS_NAMESPACE, false)) {
         Serial.println("!!! CRITICAL ERROR: Failed to initialize NVS!");
         return false;
    }

    if (!preferences.isKey(NVS_KEY_MODE)) {
        Serial.println("NVS mode key does not exist, setting to MODE_UNCONFIGURED.");
        preferences.putUInt(NVS_KEY_MODE, MODE_UNCONFIGURED);
    }
    preferences.end(); 
    return true;
}

// --- NVS Configuration Management ---

/**
 * @brief Loads the configuration from NVS into global variables.
 * @return true if configuration was loaded successfully and the device mode
 * stored in NVS was MODE_CONFIGURED and a valid SSID was found.
 * Returns false otherwise (NVS error, mode unconfigured, or missing SSID).
 */
bool loadConfigurationFromNVS() {
    if (!preferences.begin(NVS_NAMESPACE, true)) { 
         Serial.println("!!! ERROR: Failed to open NVS in read mode while loading configuration!");
         return false; 
    }

    currentDeviceMode = (DeviceMode)preferences.getUInt(NVS_KEY_MODE, MODE_UNCONFIGURED);

    if (currentDeviceMode == MODE_CONFIGURED) {
        wifiSSID = preferences.getString(NVS_KEY_SSID, "");
        wifiPass = preferences.getString(NVS_KEY_PASS, "");
        userName = preferences.getString(NVS_KEY_USER, "defaultUser");
        serverAddress = preferences.getString(NVS_KEY_SERVER, "192.168.50.23:5000"); // Use default if missing

        preferences.end();

        if (wifiSSID.length() > 0) {
            Serial.println("Loaded configuration from NVS:");
            Serial.printf("  SSID: %s\n", wifiSSID.c_str());
            // Avoid printing password for security: Serial.printf("  Pass: %s\n", wifiPass.c_str());
            Serial.printf("  User: %s\n", userName.c_str());
            Serial.printf("  Server: %s\n", serverAddress.c_str());
            return true; 
        } else {
             Serial.println("Configured mode, but SSID is missing in NVS. Forcing AP mode.");
             return false;
        }
    } else {
        Serial.println("Device in unconfigured mode (as per NVS).");
        preferences.end(); 
        return false; 
    }
}

/**
 * @brief Saves the current configuration (from global variables wifiSSID, wifiPass, etc.) to NVS.
 * Sets the device mode to MODE_CONFIGURED in NVS.
 * Updates the currentDeviceMode global variable.
 */
void saveConfigurationToNVS() {
    if (!preferences.begin(NVS_NAMESPACE, false)) { 
        Serial.println("!!! ERROR: Failed to open NVS in write mode!");
        return;
    }
    preferences.putString(NVS_KEY_SSID, wifiSSID);
    preferences.putString(NVS_KEY_PASS, wifiPass);
    preferences.putString(NVS_KEY_USER, userName);
    preferences.putString(NVS_KEY_SERVER, serverAddress);
    preferences.putUInt(NVS_KEY_MODE, MODE_CONFIGURED); 
    preferences.end(); 
    Serial.println("Configuration saved to NVS.");
    currentDeviceMode = MODE_CONFIGURED; 
}


/**
 * @brief Clears the configuration in NVS by setting the mode to UNCONFIGURED.
 * Optionally removes other keys for completeness.
 * Updates the currentDeviceMode global variable and clears sensitive RAM variables.
 */
void clearConfigurationInNVS() {
    if (!preferences.begin(NVS_NAMESPACE, false)) { 
        Serial.println("!!! ERROR: Failed to open NVS in write mode during clearing!");
        return;
    }
    preferences.putUInt(NVS_KEY_MODE, MODE_UNCONFIGURED);
    preferences.remove(NVS_KEY_SSID);
    preferences.remove(NVS_KEY_PASS);
    preferences.remove(NVS_KEY_USER);
    preferences.remove(NVS_KEY_SERVER);
    preferences.end();
    Serial.println("NVS configuration cleared (mode set to unconfigured).");
    currentDeviceMode = MODE_UNCONFIGURED; 
    wifiSSID = "";
    wifiPass = "";
}