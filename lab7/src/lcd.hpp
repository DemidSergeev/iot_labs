#ifndef LCD_HPP
#define LCD_HPP

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

namespace lcd {

// --- CONFIGURATION ---
// Set to 'false' to silence the Serial Monitor simulation
const bool print_to_serial = true; 

// Adjust 0x27 to 0x3F if your screen doesn't work
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom char: Hourglass / Clock icon
byte clockChar[] = {
  0x1F, 0x11, 0x0A, 0x04, 0x04, 0x0A, 0x1F, 0x00
};

void init() {
    if (print_to_serial) Serial.println("[LCD] Initializing Display (Hardware)...");
    
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, clockChar);
    
    lcd.setCursor(0, 0);
    lcd.print("Booting System..");
    
    if (print_to_serial) Serial.println("[LCD SIM] Booting System...");
}

void printStatus(String line1, String line2) {
    // Hardware
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    // Serial Simulation
    if (print_to_serial) {
        Serial.println("-----------------------------");
        Serial.println("[LCD SIM] " + line1);
        Serial.println("[LCD SIM] " + line2);
        Serial.println("-----------------------------");
    }
}

/**
 * @brief Updates the display with the countdown
 */
void updateCountdown(int days, int hours, int minutes, int seconds, int targetYear) {
    // Hardware
    lcd.setCursor(0, 0);
    lcd.write(0); // Clock icon
    lcd.print(" ");
    lcd.print(targetYear);
    lcd.print(" in ");

    char buffer[17]; 
    snprintf(buffer, sizeof(buffer), "%3dd %02d:%02d:%02d", days, hours, minutes, seconds);

    lcd.setCursor(0, 1);
    lcd.print(buffer);

    // Serial Simulation
    if (print_to_serial) {
        // We use \r to update the same line in terminal
        Serial.printf("[LCD SIM] New Year %d in %s   \r", targetYear, buffer);
    }
}

void showHappyNewYear(int year) {
    // Hardware
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("HAPPY  NEW");
    lcd.setCursor(4, 1);
    lcd.print("YEAR ");
    lcd.print(year);

    // Serial Simulation
    if (print_to_serial) {
        Serial.println("\n\n****************************");
        Serial.printf("[LCD SIM] HAPPY NEW YEAR %d! \n", year);
        Serial.println("****************************\n");
    }
}

} // namespace lcd
#endif // LCD_HPP