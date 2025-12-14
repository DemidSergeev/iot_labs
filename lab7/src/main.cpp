#include <Arduino.h>
#include "sta.hpp"
#include "ntp.hpp"
#include "lcd.hpp"

// --- Configuration ---
const int TARGET_YEAR = 2026;

void setup() {
    Serial.begin(115200);
    
    // 1. Init LCD
    lcd::init();

    // 2. Connect WiFi
    lcd::printStatus("Connecting to", "WiFi Network");
    if (!sta::connect_to_wifi()) {
        lcd::printStatus("WiFi Failed", "Check Credentials");
        while(1) delay(1000);
    }

    // 3. Sync Time
    lcd::printStatus("Syncing Time on", "NTP Server");
    ntp::init();
    
    // Wait for time sync
    int retry = 0;
    while (!ntp::isTimeSet() && retry < 20) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    
    if (ntp::isTimeSet()) {
        Serial.println("\n[MAIN] Time synchronized.");
        struct tm current_tm = ntp::getCurrentTime();
            Serial.printf("[MAIN] Current Local Time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                          current_tm.tm_year + 1900, current_tm.tm_mon + 1, current_tm.tm_mday,
                          current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);
        lcd::printStatus("Ready!", "Starting Timer");
        delay(2000);
        lcd::lcd.clear();
    } else {
        lcd::printStatus("NTP Error", "Retrying...");
    }
}

void loop() {
    // 1. Get current time
    time_t now;
    time(&now); // Get current timestamp (seconds since epoch)

    // 2. Define Target Time (Jan 1, TARGET_YEAR, 00:00:00)
    struct tm target_tm = {0};
    target_tm.tm_year = TARGET_YEAR - 1900; // Years since 1900
    target_tm.tm_mon  = 0;                  // January (0-11)
    target_tm.tm_mday = 1;                  // 1st
    target_tm.tm_hour = 0;
    target_tm.tm_min  = 0;
    target_tm.tm_sec  = 0;
    target_tm.tm_isdst = -1; // Let mktime determine DST

    // Convert target structure to timestamp
    // Note: mktime uses the local timezone configured in ntp
    time_t target_timestamp = mktime(&target_tm);

    // 3. Calculate difference
    double seconds_diff = difftime(target_timestamp, now);

    // 4. Update Display
    if (seconds_diff > 0) {
        // Calculate breakdown
        long remaining = (long)seconds_diff;
        
        int days = remaining / 86400;
        remaining %= 86400;
        
        int hours = remaining / 3600;
        remaining %= 3600;
        
        int minutes = remaining / 60;
        int seconds = remaining % 60;

        lcd::updateCountdown(days, hours, minutes, seconds, TARGET_YEAR);
    } else {
        // It is New Year!
        lcd::showHappyNewYear(TARGET_YEAR);
    }

    // Update every second
    delay(1000);
}