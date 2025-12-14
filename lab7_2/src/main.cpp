#include <Arduino.h>
#include "sta.hpp"
#include "ntp.hpp"
#include "lcd.hpp"
#include "rfid.hpp"
#include "event_manager.hpp"

void setup() {
    Serial.begin(115200);
    while(!Serial) delay(10);

    // 1. Initialize Hardware
    lcd::init();
    rfid::init();

    // 2. Connect Network
    lcd::printStatus("Connecting...", "WiFi Network");
    if (!sta::connect_to_wifi()) {
        lcd::printStatus("WiFi Failed", "Check Credentials");
        while(1) delay(1000);
    }

    // 3. Sync Time
    lcd::printStatus("Syncing Time...", "NTP Server");
    ntp::init();
    
    // Blocking wait for time sync (crucial for accurate countdowns)
    while (!ntp::isTimeSet()) { 
        Serial.print("."); 
        delay(500); 
    }
    
    lcd::printStatus("System Ready", "Scan Tags...");
    delay(1500);
}

void loop() {
    // Run the application logic
    event_manager::run();

    // Short delay to prevent flooding the I2C bus and Serial
    delay(200); 
}