#include <Arduino.h>
#include "sta.hpp" 
#include "microphone.hpp"
#include "filesystem.hpp"
#include "web_server.hpp"

// A small buffer used for transferring data from the mic to the file or stream
const size_t CHUNK_BUFFER_SIZE = 1024;
uint8_t data_chunk_buffer[CHUNK_BUFFER_SIZE];

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("\n[MAIN] Initialization started.");

    // 1. Initialize LittleFS
    if (!fs::init()) {
        Serial.println("[MAIN] Filesystem initialization failed. Halting.");
        while (true) { delay(1000); }
    }

    // 2. Connect to WiFi
    if (!sta::connect_to_wifi()) {
        Serial.println("[MAIN] Error connecting to WiFi. Halting.");
        while (true) { delay(1000); }
    }

    // 3. Initialize microphone
    if (!mic::setupMic()) {
        Serial.println("[MAIN] Error initializing microphone. Halting.");
        while (true) { delay(1000); }
    }
    
    // 4. Setup web server
    web_server::setupServer();

    Serial.println("[MAIN] Setup complete. System is ready.");
    Serial.println("[MAIN] Access the web interface at: http://" + WiFi.localIP().toString() + "/");
}

void loop() {
    // This must be called on every loop to handle incoming HTTP requests.
    web_server::handleClient();

    // --- Handle Recording Logic ---
    // If the recording flag is set by the web server...
    if (web_server::isRecording()) {
        // Read a chunk of audio data from the microphone
        size_t bytes_read = mic::readChunk(data_chunk_buffer, CHUNK_BUFFER_SIZE);
        
        // If we got data, append it to the open file
        if (bytes_read > 0) {
            fs::appendChunk(data_chunk_buffer, bytes_read);
        }
    }

    // --- Handle Streaming Logic ---
    // If the streaming flag is set and we have a valid, connected client...
    if (web_server::isStreaming() && web_server::streamClientConnected()) {
        // Read a chunk of audio data from the microphone
        size_t bytes_read = mic::readChunk(data_chunk_buffer, CHUNK_BUFFER_SIZE);
        
        // If we got data, send it directly to the streaming client
        if (bytes_read > 0) {
            web_server::sendStreamChunk(data_chunk_buffer, bytes_read);
        }
    } else if (web_server::isStreaming() && web_server::streamClientDisconnected()) {
        // If streaming is enabled but client has disconnected, stop it.
        web_server::stopStreaming();
    }
}