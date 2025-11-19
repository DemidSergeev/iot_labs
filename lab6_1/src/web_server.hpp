#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <WebServer.h>
#include "microphone.hpp"

namespace web_server {

WebServer server(80);

// --- Plotter Handlers ---
void handlePlotStart() { mic::startPlotting(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); }
void handlePlotStop() { mic::stopPlotting(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); }

// --- CORRECTED, FULLY COMPLIANT WAV HEADER GENERATION ---
void createWavHeader(byte* header, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels, uint32_t dataSize) {
    memcpy(header, "RIFF", 4);
    uint32_t chunkSize = 36 + dataSize;
    header[4] = (byte)(chunkSize); header[5] = (byte)(chunkSize >> 8); header[6] = (byte)(chunkSize >> 16); header[7] = (byte)(chunkSize >> 24);
    memcpy(header + 8, "WAVE", 4);
    memcpy(header + 12, "fmt ", 4);
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
    header[20] = 1; header[21] = 0;
    header[22] = (byte)(numChannels); header[23] = (byte)(numChannels >> 8);
    header[24] = (byte)(sampleRate); header[25] = (byte)(sampleRate >> 8); header[26] = (byte)(sampleRate >> 16); header[27] = (byte)(sampleRate >> 24);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;
    header[28] = (byte)(byteRate); header[29] = (byte)(byteRate >> 8); header[30] = (byte)(byteRate >> 16); header[31] = (byte)(byteRate >> 24);
    header[32] = (byte)(blockAlign); header[33] = (byte)(blockAlign >> 8);
    header[34] = (byte)(bitsPerSample); header[35] = (byte)(bitsPerSample >> 8);
    memcpy(header + 36, "data", 4);
    header[40] = (byte)(dataSize); header[41] = (byte)(dataSize >> 8); header[42] = (byte)(dataSize >> 16); header[43] = (byte)(dataSize >> 24);
}

void handleRecord() {
    String mock = server.arg("mock");
    if (mock == "1") mic::generateMockData();
    else mic::recordToMemory();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

void handleDownload() {
    const uint8_t* buffer = mic::getRamBuffer();
    size_t size = mic::getRamBufferSize();
    if (buffer && size > 0) {
        byte header[44];
        // --- CORRECTED: Pass the correct number of channels (2) to the header function ---
        createWavHeader(header, mic::SAMPLE_RATE, mic::BITS_PER_SAMPLE, mic::NUM_CHANNELS, size);
        
        server.sendHeader("Content-Disposition", "attachment; filename=ram_recording.wav");
        server.setContentLength(sizeof(header) + size);
        server.send(200, "audio/wav", "");
        server.sendContent((const char*)header, sizeof(header));
        server.sendContent((const char*)buffer, size);
    } else {
        server.send(404, "text/plain", "No RAM recording found.");
    }
}

void handleRoot() {
    String html = "<html><head><title>ESP32 Mic Control</title></head><body>";
    html += "<h1>ESP32 Microphone Interface</h1>";
    html += "<h3>RAM Recording</h3>";
    html += "<p><a href='/record?mock=0'>Record " + String(mic::RAM_REC_DURATION_S) + "s to RAM</a></p>";
    html += "<p><a href='/record?mock=1'>Generate Mock Data (Sine Wave 440 Hz)</a></p>";
    if (mic::getRamBufferSize() > 0) {
        html += "<p><b>RAM buffer has data!</b> <a href='/download'>Download RAM recording</a></p>";
    }
    html += "<hr><h3>Real-time Plotting</h3><p>Use Arduino IDE Serial Plotter (115200 baud).</p>";
    html += "<p><a href='/plot/start'>START Plotting</a></p>";
    html += "<p><a href='/plot/stop'>STOP Plotting</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void setupServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/record", HTTP_GET, handleRecord);
    server.on("/download", HTTP_GET, handleDownload);
    server.on("/plot/start", HTTP_GET, handlePlotStart);
    server.on("/plot/stop", HTTP_GET, handlePlotStop);
    server.onNotFound([](){ server.send(404, "text/plain", "Not Found"); });
    server.begin();
    Serial.println("[WEB] Web server started.");
}

void handleClient() { server.handleClient(); }

} // namespace web_server
#endif // WEB_SERVER_HPP