/**
 * @file main.cpp
 * @brief Main application file for the ESP32S3 Weather Station.
 *
 * Initializes hardware (LED, button, I2C), file systems (LittleFS, NVS),
 * Wi-Fi connectivity, and web server for configuration. It then starts
 * FreeRTOS tasks for button handling, wind sensing, and main sensor data
 * acquisition and transmission. The main loop handles web server requests
 * in unconfigured mode or checks Wi-Fi status in configured mode.
 */
#include "Arduino.h"

#include "config.h"
#include "utils.h"
#include "nvs_handler.h"
#include "wifi_manager.h"
#include "web_interface.h"
#include "data_sender.h" 

#include <WiFi.h>        
#include <Wire.h>         
#include <freertos/FreeRTOS.h> 
#include <freertos/task.h>     

// --- Global Variable Definitions (declared as extern in config.h) ---
String wifiSSID = "";
String wifiPass = "";
String userName = "defaultUser";       // Default username
String serverAddress = "192.168.50.200:5000"; // Default server address
DeviceMode currentDeviceMode = MODE_UNCONFIGURED; 
unsigned long lastDataSendTime = 0;
bool bmeSensorOk = false; 

/**
 * @brief Setup function, runs once on ESP32S3 startup.
 * Initializes serial communication, hardware components (LED, button),
 * file systems, I2C bus, loads configuration from NVS, attempts Wi-Fi connection
 * or starts AP mode, and creates FreeRTOS tasks for core functionalities.
 */
void setup() {
    Serial.begin(115200);
    while (!Serial); 
    Serial.println("\n\n === Starting ESP32S3 Weather Station ===");

    setupLed();    
    setupButton();

    if (!initLittleFS()) {
        while(1) { delay(1000); }
    }
    if (!initNVS()) {
        blinkLedError(red); 
        while(1) { delay(1000); }
    }

    // --- I2C Initialization ---
    Serial.println("Initializing I2C bus...");
    Wire.begin(I2C_SDA, I2C_SCL); 
    // Wire.setClock(100000); // Optionally set I2C clock speed if needed

    // Start the button handling task
    xTaskCreatePinnedToCore(
        buttonTask, "ButtonTask", 2048, NULL, 1, NULL, 1);
    Serial.println("Button handling task started.");

    // --- Startup Logic: Load NVS Configuration or Start AP Mode ---
    if (loadConfigurationFromNVS()) {
         Serial.println("Configuration found in NVS. Attempting to connect to WiFi...");
         WiFi.disconnect(true); 
         delay(100);
         if (connectToWiFi()) {
             sendMacAddress(); 
         } else {
             Serial.println("Automatic WiFi connection failed. Switching to AP mode for configuration.");
             clearConfigurationInNVS(); 
             switchToAPMode();
             setupWebServer();
         }
    } else {
         Serial.println("No valid configuration in NVS or device set to unconfigured. Starting in AP mode.");
         clearConfigurationInNVS();
         switchToAPMode();
         setupWebServer(); 
    }

    // --- Create FreeRTOS Tasks for Sensor Data Handling ---
    Serial.println("Creating Wind Sensor Task...");
    xTaskCreatePinnedToCore(
        windSensorTaskFunction,   
        "WindSensorTask",        
        4096,                    
        NULL,                     
        2,                      
        NULL,                     
        APP_CPU_NUM               
    );

    Serial.println("Creating Main Sensor Task...");
    xTaskCreatePinnedToCore(
        sensorTaskFunction,       
        "SensorDataTask",        
        8192,                     
        NULL,                     
        1,                        
        NULL,                     
        APP_CPU_NUM               
    );
    
    Serial.println("Sensor reading and sending task created.");
    Serial.println("Setup finished.");
}

/**
 * @brief Main loop function, runs repeatedly after setup.
 * If the device is in MODE_UNCONFIGURED, it handles web server client requests.
 * If in MODE_CONFIGURED, it checks and maintains the Wi-Fi connection.
 * Sensor data reading and transmission are handled by dedicated FreeRTOS tasks.
 */
void loop() {
    if (currentDeviceMode == MODE_UNCONFIGURED) {
        handleWebServerClient(); 
    } else if (currentDeviceMode == MODE_CONFIGURED) {
        checkAndReconnectWiFi(); 
    }
    vTaskDelay(pdMS_TO_TICKS(20)); 
}