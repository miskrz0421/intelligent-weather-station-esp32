/**
 * @file web_interface.h
 * @brief Declarations for web server interface management functions.
 *
 * This header file provides function prototypes for setting up, handling client
 * requests, and stopping the web server used for device configuration when
 * in Access Point (AP) mode.
 */
#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "config.h" 

/**
 * @brief Configures and starts the web server.
 * Defines HTTP endpoints for configuration via web interface.
 */
void setupWebServer();

/**
 * @brief Handles incoming client requests on the web server.
 * @note Should be called repeatedly in the main loop() when the server is active (AP mode).
 */
void handleWebServerClient();

/**
 * @brief Stops the web server and the mDNS service.
 * Usually called before switching from AP mode to STA mode.
 */
void stopWebServerAndMDNS();


#endif // WEB_INTERFACE_H