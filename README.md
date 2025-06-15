# ESP32-S3 Intelligent Weather Station 

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE) <!-- Optional: Add a license badge -->

This repository contains the firmware for an ESP32-S3 based intelligent weather station. The device collects various meteorological data points, processes them, and transmits them to a server. This data is intended to be used by a separate backend system featuring a Machine Learning model for classifying current weather conditions.

## Overview

The ESP32-S3 module serves as the primary data acquisition unit. It reads data from a suite of sensors, including temperature, humidity, barometric pressure, ambient light, rainfall, and wind speed. The firmware handles Wi-Fi connectivity, configuration through a web portal in Access Point (AP) mode, and periodic data transmission to a specified server endpoint in Station (STA) mode.

The collected data is formatted as JSON and sent via HTTP POST, making it ready for storage, analysis, and as input for a weather classification model.

## Key Features

*   **Multi-Sensor Data Acquisition:**
    *   Temperature, Humidity, Pressure (BME280)
    *   Ambient Light Intensity (Photoresistor)
    *   Rain Detection (Analog Rain Sensor)
    *   Wind Speed (Analog Wind Sensor)
*   **Wi-Fi Connectivity:**
    *   **Access Point (AP) Mode:** For initial device configuration (Wi-Fi credentials, server address, username).
    *   **Station (STA) Mode:** For connecting to an existing Wi-Fi network and transmitting data.
*   **Web-Based Configuration:** User-friendly interface accessible via `192.168.4.1` in AP mode.
*   **Persistent Configuration:** Saves Wi-Fi and server settings in Non-Volatile Storage (NVS).
*   **Data Transmission:** Sends sensor readings as a JSON payload to a configurable server endpoint.
*   **Device Registration:** Sends its MAC address for registration upon first successful connection in STA mode.
*   **Status Indication:** Uses an onboard NeoPixel LED to provide visual feedback on device status.
*   **Reset Functionality:** A physical button allows resetting the configuration and reverting to AP mode.
*   **FreeRTOS Based:** Utilizes FreeRTOS tasks for efficient handling of sensor readings, data transmission, and button inputs.

## Hardware Requirements

*   ESP32-S3 Development Board
*   **BME280 Sensor:** Temperature, Humidity, Pressure (I2C: SDA Pin 8, SCL Pin 9, Address `0x76`)
*   **Photoresistor:** Ambient Light (Analog Pin `1`)
*   **Rain Sensor Module:** Analog Output (Analog Pin `2`)
*   **Analog Wind Speed Sensor:** (Analog Pin `7`)
*   **Built-in LED:** Status Indicator
*   **Push Button:** For Reset (Pin `6`, connected to GND, uses internal pull-up)
*   Appropriate wiring and power supply.

## Software & Dependencies

*   Arduino IDE or PlatformIO IDE
*   ESP32 Board Support Package
*   **Libraries (managed via Arduino Library Manager or `platformio.ini`):**
    *   `Adafruit BME280 Library`
    *   `Adafruit Unified Sensor`
    *   `NeoPixelBus` (by Makuna)
    *   `WebServer` (ESP32 built-in)
    *   `Preferences` (ESP32 built-in for NVS)
    *   `HTTPClient` (ESP32 built-in)
    *   `ArduinoJson` (by Benoit Blanchon)
    *   `WiFi` (ESP32 built-in)
    *   `LittleFS` (for ESP32)

## Installation & Setup

1.  **Clone the Repository:**
    ```bash
    https://github.com/miskrz0421/intelligent-weather-station-esp32.git
    cd intelligent-weather-station-esp32
    ```
2.  **Open in IDE:** Open the project in the Arduino IDE or PlatformIO.
3.  **Install Libraries:** Ensure all required libraries listed above are installed.
4.  **Board Selection:** Select the correct ESP32-S3 board in your IDE.
5.  **Configure Pins (if different):** Most pin configurations are in `config.h`. Adjust if your wiring differs.
6.  **Upload Firmware:** Connect the ESP32-S3 board to your computer and upload the firmware.

## Configuration

On the first boot, or after a configuration reset, the device will start in Access Point (AP) mode:

1.  **LED Indicator:** The NeoPixel LED will turn **Yellow**.
2.  **Connect to AP:** Using a phone or computer, connect to the Wi-Fi network with SSID: `ESP32_Config_AP` and Password: `12345678`.
3.  **Access Web Portal:** Open a web browser and navigate to `http://esp32-config.local`.
4.  **Enter Details:**
    *   Select your local Wi-Fi network (SSID).
    *   Enter your local Wi-Fi password.
    *   Enter a Username (for server-side identification).
    *   Enter the Server Address (e.g., `your-server-ip:port` or `your-server-domain.com`).
5.  **Submit Configuration:** Click the submit button. The ESP32 will attempt to connect to your specified Wi-Fi network.

## Operation

Once configured and successfully connected to your Wi-Fi (STA mode):

1.  **LED Indicator:** The NeoPixel LED will turn **Green**.
2.  **Device Registration:** The ESP32 will send its MAC address to the registration endpoint: `http://<serverAddress>/<username>/add_device/<mac_address>`.
3.  **Data Transmission:**
    *   The device will periodically (default: every 5 seconds, defined by `DATA_SEND_INTERVAL`) read data from all sensors.
    *   Pressure readings are reduced to Mean Sea Level (MSL) using the station altitude (`STATION_ALTITUDE_METERS = 262.0` in `data_sender.cpp`).
    *   Sensor data is compiled into a JSON payload (see example below) and sent via HTTP POST to: `http://<serverAddress>/<mac_plytki>/data`.
    *   `<mac_plytki>` is the device's MAC address.

## Machine Learning Component (Weather Classification)

This ESP32 firmware is designed to provide the raw sensor data required by a Machine Learning model for weather condition classification. The ML model itself (a hierarchical XGBoost classifier) is not part of this ESP32 firmware but is expected to run on the server-side or a separate processing unit.

The model is trained to classify weather into one of six categories:
`'Clear/Fair'`, `'Cloudy/Overcast'`, `'Fog'`, `'Rain'`, `'Snow/Sleet/Freezing'`, `'Thunderstorm/Severe'`.

## Web Interface (Separate Repository)

The user-facing web interface for displaying weather data, managing devices, and interacting with the ML classification results is hosted in a separate repository.

**Briefly, the web interface provides:**
*   A dashboard to view current and historical weather data from registered ESP32 stations.
*   A section to manage your weather stations (view, rename, delete).
*   Functionality to request a weather classification for a specific historical data point using the trained ML model.

**You can find the Web Interface repository here:**
➡️ **[https://github.com/Tomciom/ISS-Server]** ⬅️

## Button Functionality

A push button connected to Pin `6` (and GND) allows for:
*   **Resetting Configuration:** If the device is in STA mode (Green LED), pressing and holding the button will clear the stored Wi-Fi/server configuration from NVS and restart the device in AP mode (Yellow LED), allowing for reconfiguration.
*   **AP Mode Indication:** If pressed while in AP mode, it will briefly blink Yellow but won't perform a major action.

## LED Status Indicators

*   **Yellow (Solid):** Device is in Access Point (AP) mode, awaiting configuration.
*   **Blue (Solid/Blinking):** Attempting to connect to Wi-Fi in Station (STA) mode.
*   **Green (Solid):** Successfully connected to Wi-Fi in STA mode and operating normally (sending data).
*   **Red (Blinking):** Critical error (e.g., NVS/LittleFS init fail) or HTTP data transmission error.

## Troubleshooting

*   **Cannot connect to `ESP32_Config_AP`:** Ensure your Wi-Fi is enabled on your phone/computer and you are within range. Check if the ESP32 has power and the LED is Yellow.
*   **Web portal `http://esp32-config.local` not loading:** Double-check you are connected to the `ESP32_Config_AP` network. Try a different browser or device.
*   **Device not connecting to home Wi-Fi (LED not Green after config):**
    *   Verify SSID and password accuracy during configuration.
    *   Ensure your Wi-Fi router is operational and within range.
    *   Check router settings (e.g., MAC filtering, 2.4GHz band enabled).
    *   Try resetting the ESP32 configuration using the button and re-configure.
*   **Data not appearing on server:**
    *   Verify the Server Address and Username are correct in the ESP32 configuration.
    *   Check server-side logs for incoming requests or errors.
    *   Ensure the ESP32 has a Green LED, indicating successful Wi-Fi connection.
     
## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Authors

**Tomasz Madeja** - [Tomciom](https://github.com/Tomciom)

**Michał Krzempek** - [miskrz0421](https://github.com/miskrz0421)

**Kacper Machnik** - [KacperMachnik](https://github.com/KacperMachnik)
