#ifndef RTOS_HPP
#define RTOS_HPP

#include <Arduino.h>
#include "web_server.hpp"
#include "microphone.hpp"

namespace rtos {

/**
 * @brief Task for Core 0: Web Server Request Handling.
 * This task continuously checks for incoming HTTP clients.
 */
void taskCore0_WebServer(void *pvParameters) {
    delay(10);
    Serial.println("[RTOS] Web Server task on Core 0 started.");
    for (;;) {
        web_server::handleClient();
        // Give a small delay to prevent starving other low-priority tasks
        vTaskDelay(pdMS_TO_TICKS(5)); 
    }
}

/**
 * @brief Task for Core 1: Real-time Microphone Plotting.
 * This task reads from the microphone and prints to Serial when plotting is enabled.
 */
void taskCore1_MicPlotter(void *pvParameters) {
    delay(20);
    Serial.println("[RTOS] Mic Plotter task on Core 1 started.");
    for (;;) {
        // This function will only read and print if the plotting flag is true
        mic::readAndPrintSignal();
        
        // A tiny delay is crucial to allow the watchdog timer to reset
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


/**
 * @brief Initializes and starts the FreeRTOS tasks.
 */
void setupRtos() {
    Serial.println("[RTOS] RTOS setup started.");

    // Create and pin the Web Server task to Core 0
    xTaskCreatePinnedToCore(
        taskCore0_WebServer,
        "WebServer_Task",
        8192,           // Stack size in words
        NULL,
        1,              // Priority
        NULL,
        0               // Core ID
    );

    // Create and pin the Microphone Plotter task to Core 1
    xTaskCreatePinnedToCore(
        taskCore1_MicPlotter,
        "MicPlotter_Task",
        4096,           // Stack size
        NULL,
        1,              // Priority
        NULL,
        1               // Core ID
    );
}

} // namespace rtos

#endif // RTOS_HPP