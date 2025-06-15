/**
 * @file web_interface.cpp
 * @brief Manages the web server interface for ESP32 configuration.
 *
 * This file implements the web server setup, endpoint handlers for serving
 * HTML/CSS content, processing configuration form submissions (Wi-Fi credentials,
 * server details), and managing the web server lifecycle (start/stop).
 * It facilitates device configuration when in Access Point (AP) mode.
 */
#include "web_interface.h"
#include "config.h"       
#include "utils.h"        
#include "wifi_manager.h" 
#include "nvs_handler.h"  
#include "data_sender.h"  
#include <WiFi.h>         
#include <ESPmDNS.h>      

// --- Global Objects ---
WebServer server(80);

// --- Web Server Endpoint Handlers ---

/**
 * @brief Handles requests to the root path ("/").
 * Loads index.html, injects the current server address, scans for WiFi networks,
 * populates the network list, and sends the configuration page to the client.
 */
void handleRoot() {
    Serial.println("Handling request for /");
    String html = loadFile("/index.html"); 
    if (html.length() == 0) {
        server.send(500, "text/plain", "Server Error: Could not load index.html"); 
        return;
    }

    // Insert the current server address as the default value in the form
    String valueAttribute = "value=\"" + serverAddress + "\"";
    html.replace("{{SERVER_ADDRESS_VALUE}}", valueAttribute);

    // Generate WiFi list dropdown
    String wifiList = "<select name=\"ssid\" id=\"ssid\" required>"; 
    int n = WiFi.scanComplete();

    // Handle scan results
    if (n == WIFI_SCAN_RUNNING) {
        wifiList += "<option value=\"\">Skanowanie sieci...</option>"; 
    } else if (n < 0) {
        wifiList += "<option value=\"\">Błąd skanowania (" + String(n) + ")</option>"; 
        startWifiScan(); 
    } else if (n == 0) {
        wifiList += "<option value=\"\">Nie znaleziono sieci</option>"; 
         startWifiScan(); 
    } else {
        Serial.printf("Found %d networks:\n", n);
        wifiList += "<option value=\"\" disabled selected>-- Wybierz sieć --</option>"; 
        for (int i = 0; i < n; ++i) {
            String currentSSID = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
            String security;
            switch(WiFi.encryptionType(i)) {
                case WIFI_AUTH_OPEN: security = "Open"; break;
                case WIFI_AUTH_WEP: security = "WEP"; break;
                case WIFI_AUTH_WPA_PSK: security = "WPA_PSK"; break;
                case WIFI_AUTH_WPA2_PSK: security = "WPA2_PSK"; break;
                case WIFI_AUTH_WPA_WPA2_PSK: security = "WPA/WPA2_PSK"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: security = "WPA2_Ent"; break;
                case WIFI_AUTH_WPA3_PSK: security = "WPA3_PSK"; break;
                case WIFI_AUTH_WPA2_WPA3_PSK: security = "WPA2/WPA3_PSK"; break;
                default: security = "Unknown"; break;
            }
            Serial.printf("  %d: %s (%d dBm) [%s]\n", i + 1, currentSSID.c_str(), rssi, security.c_str());
            wifiList += "<option value=\"" + currentSSID + "\">" + currentSSID + " (" + String(rssi) + " dBm)</option>";
        }
    }
    wifiList += "</select>";
    html.replace("{{WIFI_LIST}}", wifiList);

    server.send(200, "text/html", html);

    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
        startWifiScan();
    }
}

/**
 * @brief Handles requests for the CSS file ("/style.css").
 * Loads style.css from LittleFS and sends it with the correct content type.
 */
void handleCss() {
    Serial.println("Handling request for /style.css");
    String css = loadFile("/style.css");
     if (css.length() == 0) {
        server.send(404, "text/plain", "File style.css not found"); 
        return;
    }
    server.send(200, "text/css", css);
}

/**
 * @brief Handles the POST request to "/connect" when the configuration form is submitted.
 * Reads SSID, password, username, and server address. Sends an intermediate HTML page
 * with a JavaScript alert (in Polish), then stops the web server and AP mode,
 * attempts to connect to the specified WiFi network, saves the configuration to NVS
 * on success, and handles connection failures by reverting to AP mode.
 */
void handleConnect() {
    Serial.println("Handling POST request for /connect");
    if (!server.hasArg("ssid") || !server.hasArg("pass") || !server.hasArg("username") || !server.hasArg("serveraddr")) {
        server.send(400, "text/plain", "Missing required form data."); 
        return;
    }
    wifiSSID = server.arg("ssid");
    wifiPass = server.arg("pass");
    userName = server.arg("username");
    serverAddress = server.arg("serveraddr");

    Serial.printf("Received data:\n SSID: %s\n Password: [HIDDEN]\n User: %s\n Server Address: %s\n",
                  wifiSSID.c_str(), userName.c_str(), serverAddress.c_str());

    String initialResponseHtml = "<!DOCTYPE html><html lang=\"pl\"><head><meta charset=\"UTF-8\"><link rel=\"stylesheet\" href=\"/style.css\"><title>Laczenie...</title></head><body><div class=\"container\">";
    initialResponseHtml += "<h1>Próba połączenia...</h1>";
    initialResponseHtml += "<p>Odebrano dane konfiguracyjne dla sieci: <strong>" + wifiSSID + "</strong>.</p>";
    initialResponseHtml += "<p>Za chwilę punkt dostępowy (AP) zostanie wyłączony, a urządzenie spróbuje połączyć się z wybraną siecią.</p>";
    initialResponseHtml += "<p><strong>Twoje urządzenie zostanie rozłączone z siecią AP ESP32.</strong></p>";
    initialResponseHtml += "<p>Obserwuj diodę LED urządzenia, aby poznać status połączenia (Zielony=OK, Żółty=Spróbuj ponownie).</p>";
    initialResponseHtml += "</div>";
    initialResponseHtml += "<script>";
    initialResponseHtml += "alert('Rozpoczynam próbę połączenia z siecią \"" + wifiSSID + "\".\\n\\nPunkt dostępowy ESP32 zostanie TERAZ wyłączony.\\n\\nTwoje urządzenie straci połączenie z tą siecią konfiguracyjną.\\n\\nKliknij OK, a następnie obserwuj diodę LED urządzenia (Zielony=OK, Żółty=Spróbuj ponownie) aby poznać wynik.');";
    initialResponseHtml += "setTimeout(function(){ window.location.href = '/'; }, 2000);"; // Attempt to redirect client after 2s
    initialResponseHtml += "</script>";
    initialResponseHtml += "</body></html>";

    server.send(200, "text/html", initialResponseHtml);
    Serial.println("Sent information page with JS alert() to the browser.");
    Serial.println("Stopping HTTP server and MDNS...");
    stopWebServerAndMDNS();
    Serial.println("HTTP server and MDNS stopped.");

    Serial.println("Disconnecting AP and attempting connection in STA mode...");
    if (connectToWiFi()) { // connectToWiFi handles LED status and WiFi mode changes
        saveConfigurationToNVS();
        sendMacAddress();
    } else {
        // Connection failed
        Serial.println("Failed to connect. Returning to AP mode.");
        clearConfigurationInNVS(); 
        switchToAPMode();
        setupWebServer(); 
    }
}

// --- Web Server Management ---

/**
 * @brief Configures and starts the web server in AP mode.
 * Sets up handlers for the root path ("/"), CSS file ("/style.css"),
 * and the connection form submission ("/connect"). Includes a 404 handler.
 */
void setupWebServer() {
    Serial.println("Configuring Web Server...");
    server.on("/", HTTP_GET, handleRoot);
    server.on("/style.css", HTTP_GET, handleCss);
    server.on("/connect", HTTP_POST, handleConnect);
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not Found"); 
    });

    server.begin();
    Serial.println("HTTP server started.");
}

/**
 * @brief Handles incoming web server client requests.
 * This function should be called repeatedly in the main loop()
 * when the device is in AP mode (MODE_UNCONFIGURED).
 */
void handleWebServerClient() {
    server.handleClient();
}

/**
 * @brief Stops the web server and the mDNS service.
 * Called before attempting to switch from AP to STA mode.
 */
void stopWebServerAndMDNS() {
    Serial.println("Stopping Web Server and MDNS...");
    server.stop();
    MDNS.end(); 
    Serial.println("Web Server and MDNS stopped.");
}