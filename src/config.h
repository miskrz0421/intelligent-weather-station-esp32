/**
 * @file config.h
 * @brief Configuration settings and global variables for the ESP32 weather station project.
 *
 * This file defines constants for hardware pin assignments, sensor thresholds,
 * network configuration, API endpoints, NVS keys, device operational modes,
 * and extern declarations for global objects.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <NeoPixelBusLg.h>
#include <Adafruit_BME280.h>
#include <WebServer.h>
#include <Preferences.h>
#include <NeoPixelBus.h>

// --- NeoPixel Configuration ---
const uint16_t PixelCount = 1;
const uint8_t PixelPin = 48;
extern NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip;
extern RgbColor red;
extern RgbColor green;
extern RgbColor black;
extern RgbColor blue;
extern RgbColor yellow;

// --- Button Configuration ---
const int BUTTON_PIN = 6;

// --- WiFi Access Point Settings ---
// Credentials for the ESP32's Access Point mode, used for initial device configuration.
const char* const AP_SSID = "ESP32_Config_AP";
const char* const AP_PASS = "12345678";

// --- Sensor Configuration ---
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_ADDRESS 0x76 // I2C address for the BME280 sensor

#define PHOTORESISTOR_PIN 1
const int DARK_THRESHOLD = 500;
const int BRIGHT_THRESHOLD = 3000;
extern bool bmeSensorOk; // Flag indicating if BME280 sensor initialized successfully.

/**
 * @brief Rain sensor analog pin and moisture thresholds.
 * @note These thresholds typically require calibration for optimal performance based on the specific sensor and environment.
 */
#define RAIN_SENSOR_ANALOG_PIN 2
const int WET_THRESHOLD = 500;  // Lower analog values indicate more moisture/rain.
const int DRY_THRESHOLD = 4000; // Higher analog values indicate dry conditions.


// --- API and Network Configuration ---
extern String wifiSSID;
extern String wifiPass;
extern String userName;
extern String serverAddress;
// API endpoint paths. Placeholders like <username> and <mac_address> are replaced dynamically.
const String apiRegisterPath = "/<username>/add_device/<mac_address>";
const String apiDataPath = "/<mac_plytki>/data"; // <mac_plytki> is placeholder for device MAC

// --- Global Variables ---
const long DATA_SEND_INTERVAL = 5000; // Interval in milliseconds for sending data.
extern unsigned long lastDataSendTime;

// --- NVS Keys ---
// Keys used for storing configuration in Non-Volatile Storage (NVS).
const char* const NVS_NAMESPACE = "config";
const char* const NVS_KEY_SSID = "wifi_ssid";
const char* const NVS_KEY_PASS = "wifi_pass";
const char* const NVS_KEY_USER = "username";
const char* const NVS_KEY_SERVER = "server_addr";
const char* const NVS_KEY_MODE = "device_mode";

// --- Device States ---
/** @brief Defines the operational modes of the device. */
enum DeviceMode {
  MODE_UNCONFIGURED = 0, ///< Device requires initial WiFi and server configuration.
  MODE_CONFIGURED = 1    ///< Device is configured and operating in its normal data-logging mode.
};
extern DeviceMode currentDeviceMode;

// --- Global Objects (Extern Declarations) ---
extern Adafruit_BME280 bme;
extern WebServer server;
extern Preferences preferences;

// GPIO pin for the wind sensor input. Ensure this is an unused GPIO.
#define WIND_SENSOR_PIN 7


#endif // CONFIG_H