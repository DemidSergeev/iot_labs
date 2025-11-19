#include <Arduino.h>
#include "sta.hpp" 
#include "microphone.hpp"
#include "web_server.hpp"
#include "rtos.hpp"       // Include the new RTOS module

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("[MAIN] Initialization started.");

    // 1. Connect to WiFi
    if (!sta::connect_to_wifi()) {
        Serial.println("[MAIN] Error connecting to WiFi. Halting.");
        while (true) { delay(1000); }
    }

    // 2. Initialize microphone
    if (!mic::setupMic()) {
        Serial.println("[MAIN] Error initializing microphone. Halting.");
        while (true) { delay(1000); }
    }
    
    // 3. Setup web server
    web_server::setupServer();
    
    // 4. Setup and start RTOS tasks
    rtos::setupRtos();

    Serial.println("[MAIN] Setup complete. Handing control to RTOS.");
    // The setup task is no longer needed, so we delete it.
    vTaskDelete(NULL);
}

void loop() {
  // This loop will never be reached because the setup task is deleted.
  // All work is now done by the RTOS tasks.
}