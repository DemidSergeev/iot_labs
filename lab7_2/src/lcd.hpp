#ifndef LCD_HPP
#define LCD_HPP

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

namespace lcd {

// --- CONFIGURATION ---
// Set to 'false' to silence the Serial Monitor simulation
const bool print_to_serial = true; 

// --- State Variables ---
unsigned long ui_message_unlock_time = 0;

// Adjust 0x27 to 0x3F if your screen doesn't work
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom char: Hourglass / Clock icon
byte clockChar[] = {
  0x1F, 0x11, 0x0A, 0x04, 0x04, 0x0A, 0x1F, 0x00
};

void init() {
    Wire.begin(); 
    Wire.setTimeOut(10); 

    if (print_to_serial) {
        Serial.println("[LCD] Initializing Display (Hardware)...");
    }
    
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, clockChar);
    
    lcd.setCursor(0, 0);
    lcd.print("Booting System..");
    
    if (print_to_serial) {
        Serial.println("[LCD] Booting System...");
    }
}

void printStatus(String line1, String line2, bool serial_return_carriage = false) {
    // Hardware
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    // Serial Simulation
    if (print_to_serial) {
        Serial.printf("[LCD] %-20s | %-20s%s", line1.c_str(), line2.c_str(), serial_return_carriage ? "\r" : "\n");
    }
}

// --- Helper: Show a message and lock the UI for X seconds ---
void showMessage(String line1, String line2, int duration_ms) {
    lcd::printStatus(line1, line2, true);
    // Set the future time when the UI is allowed to update again
    ui_message_unlock_time = millis() + duration_ms;
}

} // namespace lcd
#endif // LCD_HPP