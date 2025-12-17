#include <Arduino.h>
#include "sta.hpp"
#include "ntp.hpp"
#include "lcd.hpp"
#include "rfid.hpp"
#include "event_manager.hpp"
#include "ota.hpp"

void setup() {
    Serial.begin(115200);
    while(!Serial) delay(10);

    // 1. Initialize hardware
    lcd::init();
    rfid::init();
    event_manager::init();

    // 2. Connect to network
    lcd::printStatus("Connecting to WiFi", sta::ssid);
    if (!sta::connect_to_wifi()) {
        lcd::printStatus("WiFi conn. failed", "Check Credentials");
        while(1) delay(1000);
    }

    // 3. Initialize NTP (sync time)
    lcd::printStatus("Syncing Time by NTP", ntp::server);
    ntp::init();
    
    // Blocking wait for time sync (crucial for accurate countdowns)
    while (!ntp::isTimeSet()) { 
        Serial.print("."); 
        delay(500); 
    }
    
    Serial.println("\n[MAIN] Time synchronized.");
    struct tm current_tm = ntp::getCurrentTime();
    Serial.printf("[MAIN] Current Local Time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                    current_tm.tm_year + 1900, current_tm.tm_mon + 1, current_tm.tm_mday,
                    current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);

    // 4. Initialize OTA
    ota::init();

    lcd::printStatus("System is ready", "Scan some tags!");
}

void loop() {
    // Handle OTA updates
    ota::run();
    // Run the application logic
    event_manager::run();
}