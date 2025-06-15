/**
 * @file data_sender.cpp
 * @brief Handles sensor data acquisition, processing, and transmission to a remote server.
 *
 * This file includes functions for initializing sensors (BME280),
 * calculating derived meteorological values (e.g., pressure reduced to mean sea level),
 * and FreeRTOS tasks for periodically reading sensor data (temperature, humidity,
 * pressure, light, wind, rain) and sending it as JSON to a configured API endpoint.
 * It also includes a function for device registration by sending its MAC address.
 */
#include "data_sender.h"
#include "config.h"         
#include "utils.h"         
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h> 
#include <freertos/task.h>    


// --- Global Objects ---
Adafruit_BME280 bme; 

// --- Physical Constants for Meteorological Calculations ---
const double G_CONST = 9.80665;              // Standard gravity [m/s^2]
const double MOLAR_MASS_AIR = 0.0289644;     // Molar mass of dry air [kg/mol]
const double UNIV_GAS_CONST = 8.31447;       // Universal gas constant [J/(mol*K)]
const double STATION_ALTITUDE_METERS = 262.0; // Altitude of the station [m]

/**
 * @brief Reduces station pressure to Mean Sea Level (MSL) pressure.
 * Uses the barometric formula, taking into account station pressure, temperature, and altitude.
 * @param station_pressure_hpa Measured pressure at the station in hectopascals (hPa).
 * @param station_temperature_c Measured temperature at the station in Celsius (°C).
 * @param station_altitude_m Altitude of the station above sea level in meters (m).
 * @return Pressure reduced to MSL in hPa, or NAN if input data is invalid.
 */
double reduceToMSL(double station_pressure_hpa, double station_temperature_c, double station_altitude_m) {
    if (std::isnan(station_pressure_hpa) || std::isnan(station_temperature_c)) {
        return NAN;
    }

    double station_temperature_k = station_temperature_c + 273.15; // Convert temperature to Kelvin
    double station_pressure_pa = station_pressure_hpa * 100.0;     // Convert station pressure to Pascals

    // Barometric formula exponent: (g * M * h) / (R * T_k)
    double exponent = (G_CONST * MOLAR_MASS_AIR * station_altitude_m) /
                      (UNIV_GAS_CONST * station_temperature_k);

    double pressure_msl_pa = station_pressure_pa * std::exp(exponent); // Calculate MSL pressure in Pascals
    double pressure_msl_hpa = pressure_msl_pa / 100.0;                 // Convert MSL pressure back to hPa

    return pressure_msl_hpa;
}

// --- Sensor Initialization ---

/**
 * @brief Initializes the BME280 sensor object.
 * @note Assumes Wire.begin() has ALREADY been called globally (e.g., in setup()).
 * This function should now only be called once at the start of the sensor task.
 * @return true if initialization was successful, false otherwise.
 */
bool initBME280() {
    if (!bme.begin(I2C_ADDRESS)) {
        Serial.println("!!! BME280 init failed!");
        return false;
    } else {
        Serial.println("BME280 init successful.");
        return true;
    }
}

// --- Data Transmission: Device Registration ---

/**
 * @brief Sends the device's MAC address to the registration API endpoint.
 * This is typically called once after a successful Wi-Fi connection in configured mode
 * to register the device with the backend server.
 */
void sendMacAddress() {
    if (WiFi.status() != WL_CONNECTED) { return; }
    WiFiClient client; HTTPClient http; String macAddress = WiFi.macAddress();
    String constructedEndpoint = "http://" + serverAddress + apiRegisterPath;
    constructedEndpoint.replace("<username>", userName); constructedEndpoint.replace("<mac_address>", macAddress);
    Serial.printf("Sending MAC to registration endpoint: %s\n", constructedEndpoint.c_str());
    http.begin(client, constructedEndpoint); http.setConnectTimeout(5000); http.setTimeout(5000);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        Serial.printf("Registration server response: %d\n", httpResponseCode); String response = http.getString();
        Serial.println("Response:"); Serial.println(response);
        if (httpResponseCode == 200 || httpResponseCode == 201) { blinkLedInfo(green, 2, green); } else { blinkLedError(green); }
    } else {
        Serial.printf("HTTP error during registration: %s\n", http.errorToString(httpResponseCode).c_str()); blinkLedError(green);
    }
    http.end();

    vTaskDelay(pdMS_TO_TICKS(20));
}

// --- FreeRTOS Task: Wind Sensor Data Acquisition ---

volatile float totalWindSpeedSum = 0.0; // Sum of wind speed readings for averaging
volatile int windReadingCount = 0;      // Number of wind speed readings taken
SemaphoreHandle_t windDataMutex;        // Mutex to protect shared wind data

/**
 * @brief FreeRTOS task function to periodically read wind speed from an analog sensor.
 * It accumulates readings and counts them for averaging by the main sensor task.
 * Uses a mutex to protect shared data (totalWindSpeedSum, windReadingCount).
 * @param pvParameters Standard FreeRTOS task parameters (unused).
 */
void windSensorTaskFunction(void *pvParameters) {
    Serial.println("Wind Sensor Task started.");
    windDataMutex = xSemaphoreCreateMutex();
    if (windDataMutex == NULL) {
        Serial.println("Error creating windDataMutex! Restarting...");
        ESP.restart(); 
    }
    for (;;) {
        int windSpeedAnalog = analogRead(WIND_SENSOR_PIN);
        // Map analog reading (0-1023 assumed) to wind speed (0-32.40 m/s)
        // Adjust mapping if ADC resolution or sensor output range differs.
        float currentWindSpeedms = constrain(map((long)windSpeedAnalog, 0, 1023, 0, 3240), 0L, 3240L) / 100.0F;

        if (xSemaphoreTake(windDataMutex, portMAX_DELAY) == pdTRUE) {
            totalWindSpeedSum += currentWindSpeedms;
            windReadingCount++;
            xSemaphoreGive(windDataMutex);
        } else {
            Serial.println("Wind Sensor Task: Could not take windDataMutex!");
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

// --- FreeRTOS Task: Main Sensor Data Acquisition and Transmission ---

/**
 * @brief FreeRTOS task function to periodically read sensor data
 * (BME280, photoresistor, rain sensor), retrieve averaged wind speed,
 * create a JSON payload, and send it to the API data endpoint.
 * Initializes BME280 once at the start.
 * Uses vTaskDelay for periodic execution based on DATA_SEND_INTERVAL.
 * @param pvParameters Standard FreeRTOS task parameters (unused).
 */
void sensorTaskFunction(void *pvParameters) {
    Serial.println("Sensor Task started. Initializing BME280...");
    bmeSensorOk = initBME280(); 
    if (!bmeSensorOk) {
        Serial.println("Sensor Task: BME280 initialization failed. Task will run but skip BME readings.");
    } else {
        Serial.println("Sensor Task: BME280 initialized successfully.");
    }

    Serial.println("Sensor Task entering main loop.");
    for (;;) {
        if (currentDeviceMode == MODE_CONFIGURED && WiFi.status() == WL_CONNECTED) {
            WiFiClient client;
            HTTPClient http;
            String macAddress = WiFi.macAddress();

            float temp = NAN, pressure = NAN, humidity = NAN;
            int analogValue = -1;
            int brightnessPercentage = -1;
            
            int rawRainAnalog = analogRead(RAIN_SENSOR_ANALOG_PIN);
            int precipitationPercentage = map(rawRainAnalog, WET_THRESHOLD, DRY_THRESHOLD, 100, 0);
            precipitationPercentage = constrain(precipitationPercentage, 0, 100);

            float averageWindSpeed = 0.0;

            // Safely read and reset wind data using mutex
            if (xSemaphoreTake(windDataMutex, portMAX_DELAY) == pdTRUE) {
                if (windReadingCount > 0) {
                    averageWindSpeed = totalWindSpeedSum / windReadingCount;
                } else {
                    averageWindSpeed = 0.0; 
                }
                totalWindSpeedSum = 0.0; 
                windReadingCount = 0;   
                xSemaphoreGive(windDataMutex);
                Serial.printf("Sensor Task: Calculated Average Wind Speed: %.2f m/s\n", averageWindSpeed);
            } else {
                Serial.println("Sensor Task: Could not take windDataMutex! Using 0 for wind speed.");
                averageWindSpeed = 0.0; 
            }
        
            // Read BME280 sensor data if initialized
            if (bmeSensorOk) {
                temp = bme.readTemperature(); 
                pressure = bme.readPressure() / 100.0F; 
                humidity = bme.readHumidity() / 100.0F;
                Serial.printf("Sensor Task: BME280 Reading: Temp=%.2f*C, Press=%.2f hPa, Hum=%.2f (0-1 scale)\n", temp, pressure, humidity);
            } else {
                Serial.println("Sensor Task: Skipping BME280 reading - sensor not initialized.");
            }

            // Read photoresistor data
            analogValue = analogRead(PHOTORESISTOR_PIN);
            brightnessPercentage = constrain(map(analogValue, BRIGHT_THRESHOLD, DARK_THRESHOLD, 100, 0), 0, 100);
            Serial.printf("Sensor Task: Photoresistor Reading: ADC=%d, Brightness=%d%%\n", analogValue, brightnessPercentage);

            // Prepare JSON payload
            StaticJsonDocument<512> jsonDocument; 
            if (!isnan(temp)) jsonDocument["temperature"] = round(temp * 100.0) / 100.0;
            
            double normalized_pressure = reduceToMSL(pressure, temp, STATION_ALTITUDE_METERS);            
            if (!isnan(normalized_pressure)) jsonDocument["pressure"]  = round(normalized_pressure * 100.0) / 100.0;
            else if (!isnan(pressure)) jsonDocument["pressure"] = round(pressure * 100.0) / 100.0; 

            if (!isnan(humidity)) jsonDocument["humidity"] = round(humidity * 10000.0) / 10000.0; 
            if (analogValue != -1) jsonDocument["sunshine"] = brightnessPercentage; else jsonDocument["sunshine"] = nullptr;
            
            jsonDocument["wind_speed"] = (round(averageWindSpeed * 3.6 * 100.0) / 100.0); 
            jsonDocument["precipitation"] = (round(precipitationPercentage * 10000.0)) / 10000.0; 
            
            String jsonData;
            serializeJson(jsonDocument, jsonData);

            // Construct API endpoint and send data
            String constructedEndpoint = "http://" + serverAddress + apiDataPath;
            String macForPath = macAddress; 
            constructedEndpoint.replace("<mac_plytki>", macForPath);

            Serial.print("Sensor Task: Sending JSON to data endpoint: ");
            Serial.print(constructedEndpoint);
            Serial.print(", Data: ");
            Serial.println(jsonData);

            http.begin(client, constructedEndpoint);
            http.addHeader("Content-Type", "application/json");
            http.setConnectTimeout(5000);
            http.setTimeout(5000);
            int httpResponseCode = http.POST(jsonData);

            if (httpResponseCode > 0) {
                Serial.printf("Sensor Task: Data server response: %d\n", httpResponseCode);
                String response = http.getString();
                Serial.println("Sensor Task: Response:");
                Serial.println(response);
                if (httpResponseCode >= 200 && httpResponseCode < 300 && WiFi.status() == WL_CONNECTED) {
                    setLedColor(green); 
                } else if (WiFi.status() == WL_CONNECTED) {
                    blinkLedError(green); 
                }
            } else {
                Serial.printf("Sensor Task: HTTP error during data sending: %s\n", http.errorToString(httpResponseCode).c_str());
                if (WiFi.status() == WL_CONNECTED) {
                    blinkLedError(green); 
                } else {
                    blinkLedError(black); 
                }
            }
            http.end();
        } else {
            Serial.println("Sensor Task: Skipping data send (not configured or not connected to WiFi).");
        }
        vTaskDelay(pdMS_TO_TICKS(DATA_SEND_INTERVAL)); 
    }
}