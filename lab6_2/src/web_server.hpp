#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <WebServer.h>
#include "microphone.hpp"
#include "filesystem.hpp"

namespace web_server {

WebServer server(80);

// --- State Management Flags ---
volatile bool record_to_file_active = false;
volatile bool streaming_active = false;

// --- Streaming Client Management ---
WiFiClient stream_client;

// --- State Access Functions (for main loop) ---
bool isRecording() { return record_to_file_active; }
bool isStreaming() { return streaming_active; }
bool streamClientConnected() { return stream_client && stream_client.connected(); }
bool streamClientDisconnected() {
    // This condition checks if we had a client that is now disconnected.
    return stream_client && !stream_client.connected();
}

// --- Streaming Functions ---
void sendStreamChunk(const uint8_t* data, size_t len) { if (stream_client) stream_client.write(data, len); }
void stopStreaming() {
    if (stream_client) {
        stream_client.stop();
        Serial.println("[WEB] Stream client disconnected.");
    }
    streaming_active = false;
}

// --- WAV Header Generation ---
void createWavHeader(byte* header, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels, uint32_t dataSize) {
    // RIFF chunk descriptor
    memcpy(header, "RIFF", 4);
    uint32_t chunkSize = 36 + dataSize;
    header[4] = (byte)(chunkSize & 0xFF);
    header[5] = (byte)((chunkSize >> 8) & 0xFF);
    header[6] = (byte)((chunkSize >> 16) & 0xFF);
    header[7] = (byte)((chunkSize >> 24) & 0xFF);
    memcpy(header + 8, "WAVE", 4);

    // "fmt " sub-chunk
    memcpy(header + 12, "fmt ", 4);
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0; // Sub-chunk size (16 for PCM)
    header[20] = 1; header[21] = 0; // Audio format (1 for PCM)
    
    // Number of Channels (16-bit)
    header[22] = (byte)(numChannels & 0xFF);
    header[23] = (byte)((numChannels >> 8) & 0xFF);
    
    // Sample Rate (32-bit)
    header[24] = (byte)(sampleRate & 0xFF);
    header[25] = (byte)((sampleRate >> 8) & 0xFF);
    header[26] = (byte)((sampleRate >> 16) & 0xFF);
    header[27] = (byte)((sampleRate >> 24) & 0xFF);

    // Block Align and Byte Rate (derived from other parameters)
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;

    // Byte Rate (32-bit)
    header[28] = (byte)(byteRate & 0xFF);
    header[29] = (byte)((byteRate >> 8) & 0xFF);
    header[30] = (byte)((byteRate >> 16) & 0xFF);
    header[31] = (byte)((byteRate >> 24) & 0xFF);
    
    // Block Align (16-bit)
    header[32] = (byte)(blockAlign & 0xFF);
    header[33] = (byte)((blockAlign >> 8) & 0xFF);
    
    // Bits per Sample (16-bit)
    header[34] = (byte)(bitsPerSample & 0xFF);
    header[35] = (byte)((bitsPerSample >> 8) & 0xFF);

    // "data" sub-chunk
    memcpy(header + 36, "data", 4);
    header[40] = (byte)(dataSize & 0xFF);
    header[41] = (byte)((dataSize >> 8) & 0xFF);
    header[42] = (byte)((dataSize >> 16) & 0xFF);
    header[43] = (byte)((dataSize >> 24) & 0xFF);
}

// --- Web Handlers ---
void handleRecordStart() {
    if (record_to_file_active) {
        server.send(409, "text/plain", "Already recording.");
        return;
    }
    String filename = "/rec_" + String(millis()) + ".raw";
    if (fs::openFileForWrite(filename)) {
        record_to_file_active = true;
        Serial.println("[WEB] Started recording to file.");
    }
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}

void handleRecordStop() {
    if (record_to_file_active) {
        fs::closeFile();
        record_to_file_active = false;
        Serial.println("[WEB] Stopped recording to file.");
    }
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}

void handleStreamStart() {
    if (streaming_active) {
        server.send(409, "text/plain", "Already streaming.");
        return;
    }
    streaming_active = true;
    Serial.println("[WEB] Streaming enabled. Waiting for client to connect to /stream endpoint.");
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}

void handleStreamStop() {
    stopStreaming();
    Serial.println("[WEB] Streaming disabled.");
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}

void handleStream() {
    if (streaming_active) {
        Serial.println("[WEB] Client connected for streaming.");
        stream_client = server.client();
        
        String header = "HTTP/1.1 200 OK\r\n";
        // Content-Type for 16-bit PCM raw audio is audio/l16
        header += "Content-Type: audio/l16;rate=" + String(mic::SAMPLE_RATE) + ";channels=" + String(mic::NUM_CHANNELS) + "\r\n";
        header += "Connection: close\r\n";
        header += "\r\n";
        
        stream_client.write(header.c_str(), header.length());
        // The main loop will now handle sending data to this client.
    } else {
        server.send(404, "text/plain", "Streaming is not currently enabled.");
    }
}

void handleDelete() {
    if (server.hasArg("file")) fs::deleteFile(server.arg("file"));
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

void handleDownload() {
    if (!server.hasArg("file")) { server.send(400, "text/plain", "Bad Request"); return; }
    String filename = server.arg("file");
    File file = LittleFS.open(filename, "r");
    if (!file) { server.send(404, "text/plain", "File Not Found"); return; }

    byte header[44];
    createWavHeader(header, mic::SAMPLE_RATE, mic::BITS_PER_SAMPLE, mic::NUM_CHANNELS, file.size());
    String downloadFilename = filename.substring(1); downloadFilename.replace(".raw", ".wav");
    
    server.sendHeader("Content-Disposition", "attachment; filename=" + downloadFilename);
    server.setContentLength(sizeof(header) + file.size());
    server.send(200, "audio/wav", "");
    server.sendContent((const char*)header, sizeof(header));
    server.streamFile(file, "application/octet-stream");
    file.close();
}

void handleRoot() {
    String html = "<html><head><title>ESP32 Mic Control</title><meta http-equiv='refresh' content='5'></head><body>";
    html += "<h1>ESP32 Microphone Interface</h1>";
    
    // --- Recording Control ---
    html += "<h3>File Recording</h3>";
    if (record_to_file_active) {
        html += "<p style='color:red;'><b>RECORDING IN PROGRESS...</b></p>";
        html += "<p><a href='/record/stop'>STOP Recording</a></p>";
    } else {
        html += "<p><a href='/record/start'>START Recording to File</a></p>";
    }

    // --- Streaming Control ---
    html += "<hr><h3>Live Streaming</h3>";
    if (streaming_active) {
        html += "<p style='color:red;'><b>STREAMING ENABLED...</b></p>";
        html += "<p>Open this URL in VLC: <b>http://" + WiFi.localIP().toString() + "/stream</b></p>";
        html += "<p><a href='/stream/stop'>STOP Streaming</a></p>";
        if (streamClientConnected()) {
            html += "<p style='color:green;'><b>Client " + stream_client.remoteIP().toString() + " connected for streaming.</b></p>";
        } else {
            html += "<p style='color:orange;'><b>No Client Connected.</b></p>";
        }
    } else {
        html += "<p><a href='/stream/start'>START Streaming</a></p>";
    }

    html += "<hr><h3>Saved Recordings</h3>" + fs::getFilesAsHTML();
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void setupServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/record/start", HTTP_GET, handleRecordStart);
    server.on("/record/stop", HTTP_GET, handleRecordStop);
    server.on("/stream/start", HTTP_GET, handleStreamStart);
    server.on("/stream/stop", HTTP_GET, handleStreamStop);
    server.on("/stream", HTTP_GET, handleStream);
    server.on("/download", HTTP_GET, handleDownload);
    server.on("/delete", HTTP_GET, handleDelete);
    server.onNotFound([](){ server.send(404, "text/plain", "Not Found"); });
    server.begin();
    Serial.println("[WEB] Web server started.");
}

void handleClient() { server.handleClient(); }

} // namespace web_server
#endif // WEB_SERVER_HPP