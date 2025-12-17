#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <Arduino.h>
#include <time.h>
#include "lcd.hpp"
#include "rfid.hpp"

namespace event_manager {

// --- Configuration ---
const int pin_boot_button = 0; // The "BOOT" button on ESP32 boards
const unsigned long button_delay_millis = 300;
const unsigned long run_interval_millis = 200;

// --- State Variables ---
unsigned long previous_run_millis = 0;
unsigned long previous_button_millis = 0;

// --- Data Structures ---
struct Event {
    String name;        // e.g., "New Year"
    int target_day;     // 1-31
    int target_month;   // 1-12
    String assignedUID; // The RFID tag ID linked to this event
};

// --- Events ---
// Initially, no UIDs are assigned ("").
Event events[] = {
    {"New Year", 1, 1, ""}, // Jan 1st
    {"Semester end", 29, 12, ""}, // Dec 29th
    {"Winter exams", 14, 1, ""}, // Jan 14th
    {"My birthday", 27, 7, ""}  // July 27th
};

const int EVENT_COUNT = sizeof(events) / sizeof(events[0]);
int currentEventIndex = 0; // Default to the first event

// --- Initialization ---
void init() {
    // GPIO 0 usually has an external pull-up, but internal doesn't hurt
    pinMode(pin_boot_button, INPUT_PULLUP);
}

// --- Helper: Calculate Target Timestamp (Handles "Next Year" logic) ---
time_t getNextTargetTimestamp(int day, int month) {
    time_t now;
    time(&now);
    struct tm* current_tm = localtime(&now);

    struct tm target_tm = {0};
    target_tm.tm_hour = 0; target_tm.tm_min = 0; target_tm.tm_sec = 0;
    target_tm.tm_isdst = -1; // Auto-detect DST
    
    target_tm.tm_mday = day;
    target_tm.tm_mon  = month - 1; // struct tm uses 0-11 for months
    
    // 1. Assume the event is this year
    target_tm.tm_year = current_tm->tm_year;
    
    time_t target_ts = mktime(&target_tm);

    // 2. If the target is in the past, move it to next year
    if (difftime(target_ts, now) < 0) {
        target_tm.tm_year += 1;
        target_ts = mktime(&target_tm);
    }
    
    return target_ts;
}

// --- Logic: Handle Button Press ---
void checkButton() {
    // Button is usually Active LOW (0 when pressed)
    if (digitalRead(pin_boot_button) == LOW) {
        // Simple Debounce
        unsigned long current_millis = millis();
        if (current_millis - previous_button_millis >= button_delay_millis) {
            previous_button_millis = current_millis;
            
            // Cycle to next event
            currentEventIndex = (currentEventIndex + 1) % EVENT_COUNT;
            
            lcd::showMessage("Switch by button", 
                "#" + String(currentEventIndex + 1) + ": " + events[currentEventIndex].name, 1000
            );
        }
    }
}

// --- Logic: Handle RFID Scans ---
void processRFID() {
    if (millis() < lcd::ui_message_unlock_time) return;

    String newUID = rfid::scanCard();
    
    if (newUID == "") return; // No card detected

    Serial.println("[LOGIC] Scanned Tag: " + newUID);
    
    // 1. Check if this tag is already known
    for (int i = 0; i < EVENT_COUNT; i++) {
        if (events[i].assignedUID == newUID) {
            currentEventIndex = i;
            Serial.println("[LOGIC] Known tag. Switching to: " + events[i].name);
            lcd::showMessage("Mode Switched:", events[i].name, 1500);
            return;
        }
    }

    // 2. If unknown, look for an empty slot to assign it to
    for (int i = 0; i < EVENT_COUNT; i++) {
        if (events[i].assignedUID == "") {
            events[i].assignedUID = newUID;
            currentEventIndex = i;
            Serial.println("[LOGIC] Learned new tag for: " + events[i].name);
            lcd::showMessage("Tag Linked!", events[i].name, 1500);
            return;
        }
    }
    
    // 3. If all slots full and tag is unknown, just cycle manually
    currentEventIndex = (currentEventIndex + 1) % EVENT_COUNT;
    lcd::showMessage("Cycling Mode...", events[currentEventIndex].name, 1000);
}

// --- Logic: Update Display ---
void updateDisplay() {
    // 1. Check if the UI is "Locked" by a previous message
    if (millis() < lcd::ui_message_unlock_time) {
        // Do not overwrite current message with the clock yet.
        return; 
    }

    // 2. Only update clock every 200ms
    if (millis() - previous_run_millis < run_interval_millis) {
        return;
    }
    time_t now;
    time(&now);
    
    Event& e = events[currentEventIndex];
    time_t target = getNextTargetTimestamp(e.target_day, e.target_month);
    double diff = difftime(target, now);
    
    // Calculate display year
    struct tm* target_struct = localtime(&target);
    int targetYear = target_struct->tm_year + 1900;

    if (diff > 0) {
        long rem = (long)diff;
        int d = rem / 86400; rem %= 86400;
        int h = rem / 3600;  rem %= 3600;
        int m = rem / 60;
        int s = rem % 60;
        
        char line1[21];
        char line2[21];
        
        // Format: "Name       2026"
        snprintf(line1, 20, "%-14s %d", e.name.c_str(), targetYear);
        // Format: "in 123d 12:00:00"
        snprintf(line2, 20, "in %3dd %02d:%02d:%02d", d, h, m, s);
        
        lcd::printStatus(String(line1), String(line2), true);
    } else {
        lcd::printStatus("EVENT REACHED!", e.name, true);
    }
}

// --- Main Public Function ---
void run() {
    checkButton();
    processRFID();
    updateDisplay();
}

} // namespace event_manager
#endif