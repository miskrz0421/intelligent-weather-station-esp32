/**
 * @file utils.cpp
 * @brief Utility functions for LED control, file system operations, and button handling.
 *
 * This file implements functions for initializing and controlling the NeoPixel LED,
 * initializing the LittleFS file system and loading files from it, and a FreeRTOS
 * task for handling button presses to switch device modes or trigger actions.
 */
#include "utils.h"
#include "config.h"      
#include <LittleFS.h>
#include <NeoPixelBus.h> 
#include <WiFi.h>        

// --- Global Object & Color Definitions ---
// Definition of the LED strip object
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip(PixelCount, PixelPin);
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor black(0, 0, 0);
RgbColor blue(0, 0, 255);
RgbColor yellow(255, 165, 0); 

// --- Function Implementations ---

// --- LED Control ---

/**
 * @brief Initializes the NeoPixel LED strip.
 * Turns the LED off initially by setting it to black.
 */
void setupLed() {
    strip.Begin();
    strip.Show(); // Initialize strip to black (off)
}

/**
 * @brief Sets the color of the single NeoPixel LED.
 * @param color The RgbColor to set.
 */
void setLedColor(RgbColor color) {
    strip.SetPixelColor(0, color);
    strip.Show();
}

/**
 * @brief Blinks the LED red 3 times to indicate an error, then sets a final color.
 * @param finalColor The color to set the LED to after blinking.
 */
void blinkLedError(RgbColor finalColor) {
    for (int i = 0; i < 3; i++) {
        setLedColor(red);
        delay(150);
        setLedColor(black);
        delay(150);
    }
    setLedColor(finalColor);
}

/**
 * @brief Blinks the LED with a specified color and number of times, then sets a final color.
 * Used for informational signals.
 * @param blinkColor The color to blink.
 * @param times The number of times to blink.
 * @param finalColor The color to set the LED to after blinking.
 */
void blinkLedInfo(RgbColor blinkColor, int times, RgbColor finalColor) {
     for (int i = 0; i < times; i++) {
        setLedColor(blinkColor);
        delay(200);
        setLedColor(black);
        delay(200);
    }
    setLedColor(finalColor);
}


// --- File System ---

/**
 * @brief Initializes the LittleFS file system.
 * Blinks red on critical failure to mount the file system.
 * @return true if LittleFS was mounted successfully, false otherwise.
 */
bool initLittleFS() {
    if (!LittleFS.begin()) {
        Serial.println("!!! CRITICAL ERROR: Failed to mount LittleFS!");
        blinkLedError(red); 
        return false;
    }
    Serial.println("LittleFS mounted.");
    return true;
}

/**
 * @brief Loads the content of a file from LittleFS into a String.
 * Uses std::unique_ptr for safer memory management of the read buffer.
 * @param path The full path to the file in LittleFS (e.g., "/index.html").
 * @return String containing the file content, or an empty String on failure or if file is empty.
 */
String loadFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file for reading: %s\n", path);
        return String(); 
    }
    size_t size = file.size();
    if (size > 20480) { 
         Serial.printf("!!! WARNING: File %s is very large (%d bytes). May run out of memory.\n", path, size);
    }
    std::unique_ptr<char[]> buf(new char[size + 1]);
    file.readBytes(buf.get(), size);
    buf[size] = '\0'; 
    file.close();
    return String(buf.get());
}

// --- Button ---

/**
 * @brief Initializes the button pin (BUTTON_PIN from config.h) as input with an internal pull-up resistor.
 */
void setupButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

#include "nvs_handler.h"
#include "wifi_manager.h"
#include "web_interface.h" 

/**
 * @brief FreeRTOS task to handle button presses.
 * Detects a button press (connected between BUTTON_PIN and GND).
 * If pressed while the device is in MODE_CONFIGURED, it clears the NVS configuration
 * and switches the device to AP mode with the web server running for reconfiguration.
 * If pressed in MODE_UNCONFIGURED (AP mode), it performs a minor action (blinks yellow).
 * Includes debounce logic to prevent multiple triggers from a single press.
 * @param pvParameters Standard FreeRTOS task parameters (unused).
 */
void buttonTask(void *pvParameters) {
    Serial.println("Button Task started.");
    bool buttonWasPressed = false; 

    for (;;) {
        if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed (active low due to INPUT_PULLUP)
            if (!buttonWasPressed) { // Check if it's a new press event
                buttonWasPressed = true;
                vTaskDelay(pdMS_TO_TICKS(50)); // Debounce delay
                if (digitalRead(BUTTON_PIN) == LOW) { // Confirm press after debounce
                    Serial.println("Button press detected");

                    if (currentDeviceMode == MODE_CONFIGURED) {
                        Serial.println("Button pressed in configured mode -> Forcing AP mode and clearing NVS");

                        clearConfigurationInNVS(); 
                        switchToAPMode();          
                        setupWebServer();        

                    } else { // Button pressed in AP mode (MODE_UNCONFIGURED)
                        Serial.println("Button pressed in AP mode (unconfigured) - no major action taken");
                        blinkLedInfo(yellow, 1, yellow); // Visual feedback
                    }
                } else {
                     // False positive during debounce (button released quickly)
                     buttonWasPressed = false;
                }
            }
            // If button is still held down, do nothing further until released
        } else { // Button released
            if (buttonWasPressed) {
                Serial.println("Button released");
                buttonWasPressed = false; 
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}