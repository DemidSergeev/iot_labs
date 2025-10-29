#include <Arduino.h>
#include <WiFi.h>

namespace sta {

const char* ssid     = "Home2.4";
const char* password = "DontPanic42!";

bool connect_to_wifi() {
  Serial.print("[STA] Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 20) {
    delay(500);
    Serial.print(".");
    i++;
  }
  Serial.println("");
  if (i >= 20) {
    Serial.println("[STA] Connection timed out.");
    return false;
  }

  Serial.println("[STA] WiFi connected.");
  Serial.printf("[STA] IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[STA] MAC address: %s\n", WiFi.macAddress().c_str());

  return true;
}

double get_signal_strength() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[STA] Not connected to WiFi.");
    return 0.0;
  }
  int8_t rssi = WiFi.RSSI();
  if (rssi >= -50) {
    Serial.println("[STA] Signal strength: 100%");
    return 100.0;
  } else if (rssi <= -100) {
    Serial.println("[STA] Signal strength: 0%");
    return 0.0;
  } else {
    // Map RSSI (-100 to -50 dBm) to percentage (0% to 100%)
    // https://stackoverflow.com/a/15798024
    double signalStrengthPercent = map(rssi, -100, -50, 0, 100);
    Serial.printf("[STA] Signal strength: %.2f%%\n", signalStrengthPercent);
    return signalStrengthPercent;
  }
}

} // namespace sta