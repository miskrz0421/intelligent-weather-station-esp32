#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"

/**
 * @brief Switches the ESP32 to Access Point (AP) mode.
 * Configures the AP using AP_SSID/AP_PASS from config.h, starts mDNS
 * responder (esp32-config.local), sets the LED to yellow, and initiates
 * a background WiFi scan. Restarts the device on critical failure.
 */
void switchToAPMode();

/**
 * @brief Starts an asynchronous WiFi network scan in the background.
 * @param show_hidden Set to true to include hidden networks (defaults to true).
 */
void startWifiScan(bool show_hidden = true);

/**
 * @brief Attempts to connect to the WiFi network using credentials from global variables (wifiSSID, wifiPass).
 * Sets the device to STA mode. Blocks execution for up to 15 seconds during the attempt.
 * Updates the LED based on connection status (blue trying, green success, error blink failure).
 * @return true if connection is successful within the timeout, false otherwise.
 */
bool connectToWiFi();

/**
 * @brief Checks the WiFi connection status while in STA mode (MODE_CONFIGURED).
 * If disconnected, attempts to reconnect using WiFi.reconnect() for up to 10 seconds.
 * Updates the LED color accordingly. If reconnection fails, it clears NVS,
 * switches back to AP mode, starts the web server, and returns false.
 * @return true if currently connected or successfully reconnected.
 * @return false if reconnection failed (and switched to AP mode) or if not currently in STA mode (MODE_CONFIGURED).
 */
bool checkAndReconnectWiFi();


#endif // WIFI_MANAGER_H