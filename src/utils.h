/**
 * @file utils.h
 * @brief Declarations for utility functions including LED control, file system operations, and button handling.
 *
 * This header file provides function prototypes for initializing and controlling
 * the NeoPixel LED, initializing the LittleFS file system, loading files,
 * setting up the input button, and the FreeRTOS task for button press detection
 * and handling.
 */
#ifndef UTILS_H
#define UTILS_H

#include "config.h" 

// --- LED Control ---

/**
 * @brief Initializes the NeoPixel LED strip.
 */
void setupLed();

/**
 * @brief Sets the color of the single NeoPixel LED.
 * @param color The RgbColor to set.
 */
void setLedColor(RgbColor color);

/**
 * @brief Blinks the LED red 3 times to indicate an error, then sets a final color.
 * @param finalColor The color to set the LED to after blinking (defaults to black/off).
 */
void blinkLedError(RgbColor finalColor = black);

/**
 * @brief Blinks the LED with a specified color and number of times, then sets a final color.
 * @param blinkColor The color to blink.
 * @param times The number of times to blink (defaults to 1).
 * @param finalColor The color to set the LED to after blinking (defaults to black/off).
 */
void blinkLedInfo(RgbColor blinkColor, int times = 1, RgbColor finalColor = black);

// --- File System ---

/**
 * @brief Initializes the LittleFS file system.
 * @return true if LittleFS was mounted successfully, false otherwise.
 */
bool initLittleFS();

/**
 * @brief Loads the content of a file from LittleFS into a String.
 * @param path The full path to the file in LittleFS (e.g., "/index.html").
 * @return String containing the file content, or an empty String on failure.
 */
String loadFile(const char* path);

// --- Button ---

/**
 * @brief Initializes the button pin as input with internal pull-up.
 */
void setupButton();

/**
 * @brief FreeRTOS task function to handle button presses for mode switching or configuration clearing.
 * @param pvParameters Standard FreeRTOS task parameters (unused).
 */
void buttonTask(void *pvParameters); // Button handling task

#endif // UTILS_H