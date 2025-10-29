#include <Arduino.h>
#include "sta.hpp"
#include "ap.hpp"
#include "serial.hpp"

enum class Mode {
  STA = 1,
  AP = 2
};

Mode mode;

constexpr Mode operator &(Mode a, Mode b) {
    return static_cast<Mode>(static_cast<int>(a) & static_cast<int>(b));
}

constexpr Mode operator |(Mode a, Mode b) {
    return static_cast<Mode>(static_cast<int>(a) | static_cast<int>(b));
}

void setup() {
  Serial.begin(115200);
  Serial.println("Enter operating mode (1: Station, 2: Access Point, 3: Both):");

  int input;
  while ((input = serial::readInt()) < 1 || input > 3) {
    Serial.println("Invalid input. Please enter 1, 2, or 3:");
  }

  mode = static_cast<Mode>(input);
  switch (mode) {
    case Mode::STA:
      Serial.println("Operating in Station mode.");
      if (!sta::connect_to_wifi()) {
        Serial.println("Failed to connect to WiFi in Station mode.");
      }
      break;
    case Mode::AP:
      Serial.println("Operating in Access Point mode.");
      ap::create_access_point();
      break;
    case Mode::STA | Mode::AP:
      Serial.println("Operating in both Station and Access Point modes.");
      if (!sta::connect_to_wifi()) {
        Serial.println("Failed to connect to WiFi in Station mode.");
      }
      ap::create_access_point();
      break;
    default:
      Serial.println("Unknown mode selected.");
      break;
  }
}

void loop() {
    if (static_cast<int>(mode & Mode::STA)) {
        Serial.print("[STA]: ");
        sta::get_signal_strength();
    }
    if (static_cast<int>(mode & Mode::AP)) {
        Serial.print("[AP]: ");
        ap::list_connected_clients();
    }
    delay(5000);
}
