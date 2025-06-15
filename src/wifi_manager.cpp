/**
 * @file wifi_manager.cpp
 * @brief Manages Wi-Fi connectivity, including AP mode, STA mode, and network scanning.
 *
 * This file implements functions for switching the ESP32 to Access Point (AP) mode
 * for configuration, initiating Wi-Fi network scans, connecting to a specified
 * Wi-Fi network in Station (STA) mode, and handling automatic reconnection
 * if the STA connection is lost.
 */
#include "wifi_manager.h"
#include "config.h"        
#include "utils.h"         
#include "nvs_handler.h"   
#include "web_interface.h" 
#include <WiFi.h>
#include <ESPmDNS.h>

// --- WiFi Mode Switching ---

/**
 * @brief Switches the device to Access Point (AP) mode for configuration.
 * Disconnects any existing STA connection, starts a software AP with
 * credentials from config.h (AP_SSID, AP_PASS), starts mDNS responder,
 * sets LED to yellow, and triggers a background WiFi scan.
 * If AP fails to start, it blinks red and restarts the device.
 */
void switchToAPMode() {
    Serial.println("Switching to AP mode...");
    WiFi.disconnect(true); 
    WiFi.softAPdisconnect(true); 
    delay(100);

    WiFi.mode(WIFI_AP);
    bool apConfigResult = WiFi.softAP(AP_SSID, AP_PASS);

    if (apConfigResult) {
        Serial.print("AP started: ");
        Serial.println(AP_SSID);
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());

        if (MDNS.begin("esp32-config")) { // Hostname for mDNS
            Serial.println("MDNS responder started. Accessible at http://esp32-config.local");
            MDNS.addService("http", "tcp", 80);
        } else {
            Serial.println("Error starting MDNS!");
        }
        setLedColor(yellow); 
        startWifiScan();     
    } else {
        Serial.println("!!! CRITICAL ERROR: Failed to start AP mode!");
        blinkLedError(red); 
        delay(5000);
        ESP.restart();
    }
    lastDataSendTime = millis(); 
    currentDeviceMode = MODE_UNCONFIGURED; 
}

// --- WiFi Scanning ---

/**
 * @brief Starts an asynchronous WiFi network scan in the background.
 * Results are used by the web interface to populate the network list.
 * @param show_hidden Set to true to include hidden networks in the scan results.
 */
void startWifiScan(bool show_hidden /*= true*/) {
    // Parameters for scanNetworks: async=true, show_hidden=true/false, passive=false, max_ms_per_chan=default, channel=all
    WiFi.scanNetworks(true, show_hidden);
    Serial.println("Background WiFi network scan started.");
}


// --- WiFi Connection (STA Mode) ---

/**
 * @brief Attempts to connect to the WiFi network specified in the global
 * `wifiSSID` and `wifiPass` variables.
 * Sets the device to STA mode and tries connecting for up to 15 seconds.
 * Updates LED color based on connection status (blue while trying, green on success).
 * @return true if connection is successful within the timeout, false otherwise.
 */
bool connectToWiFi() {
    Serial.print("Connecting to network: ");
    Serial.println(wifiSSID);
    setLedColor(blue); 

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) { // 15 second timeout
        delay(250);
        Serial.print(".");
        setLedColor( (millis()/250) % 2 == 0 ? blue : black );
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(">>> SUCCESS: Connected to WiFi!");
        setLedColor(green);
        Serial.print("Device IP address: ");
        Serial.println(WiFi.localIP());
        lastDataSendTime = millis(); 
        currentDeviceMode = MODE_CONFIGURED; 
        return true;
    } else {
        Serial.println("!!! ERROR: Failed to connect to WiFi within the timeout.");
        blinkLedError(black);
        WiFi.disconnect(true); 
        currentDeviceMode = MODE_UNCONFIGURED; 
        return false;
    }
}

/**
 * @brief Checks the WiFi connection status while in STA mode.
 * If disconnected, it attempts to reconnect using WiFi.reconnect() for up to 10 seconds.
 * If reconnection fails, it clears the potentially invalid NVS configuration,
 * switches the device back to AP mode, starts the web server, and returns false.
 * @return true if currently connected or successfully reconnected,
 * false if reconnection failed (and switched to AP mode) or if not currently in STA mode.
 */
bool checkAndReconnectWiFi() {
    if (currentDeviceMode == MODE_CONFIGURED && WiFi.status() != WL_CONNECTED) {
        Serial.println("Lost WiFi connection in STA mode. Attempting to reconnect...");
        setLedColor(blue); 

        WiFi.reconnect();

        unsigned long reconnStart = millis();
        while(WiFi.status() != WL_CONNECTED && millis() - reconnStart < 10000) { 
            delay(250);
            Serial.print("*");
            setLedColor( (millis()/250) % 2 == 0 ? blue : black );
        }
        Serial.println();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Reconnection failed. Reverting to AP mode.");
            blinkLedError(black); 
            clearConfigurationInNVS();
            switchToAPMode();
            setupWebServer();
            return false; 
        } else {
            Serial.println("Reconnection successful.");
            setLedColor(green); 
            return true; 
        }
    } else if (currentDeviceMode == MODE_CONFIGURED && WiFi.status() == WL_CONNECTED) {
        return true;
    } else {
        return false;
    }
}