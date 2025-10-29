#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

namespace ap {

const char* ssid     = "ESP32";
const char* password = "12345678";

bool create_access_point() {
  Serial.println("Setting up Access Point...");
  WiFi.softAP(ssid, password);
  delay(1000); // Wait some time for the AP to start

  Serial.println("Access Point set up.");
  Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("AP MAC address: %s\n", WiFi.softAPmacAddress().c_str());

  return true;
}

void list_connected_clients() {
  uint8_t numClients = WiFi.softAPgetStationNum();
  if (numClients == 0) {
    Serial.println("No clients connected.");
    return;
  }

  wifi_sta_list_t clients;
  if (esp_wifi_ap_get_sta_list(&clients) == ESP_OK) {
    Serial.println("Connected clients:");
    for (int i = 0; i < numClients; i++) {
      wifi_sta_info_t client = clients.sta[i];
      Serial.printf("Client %d - MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", i + 1,
                    client.mac[0], client.mac[1], client.mac[2],
                  client.mac[3], client.mac[4], client.mac[5]);
    }
  } else {
    Serial.println("Failed to get connected clients list.");
  }
}

} // namespace ap