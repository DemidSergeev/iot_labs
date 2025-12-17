#ifndef OTA_HPP
#define OTA_HPP

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "sta.hpp"

namespace ota {

AsyncWebServer server(80);

void init() {
    Serial.println("[OTA] Initializing Async WebServer...");

    // 1. Diagnostic Endpoint
    server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Web Server is Alive!");
    });

    // 2. Redirect Root to /update
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/update");
    });

    // 3. Start ElegantOTA
    ElegantOTA.begin(&server);
    
    // 4. Start Server
    server.begin();
    
    Serial.println("[OTA] Server Ready.");
    Serial.println("      1. Test: http://" + sta::get_ip() + "/hello");
    Serial.println("      2. OTA:  http://" + sta::get_ip() + "/update");
}

void run() {
    ElegantOTA.loop();
}

} // namespace ota

#endif // OTA_HPP