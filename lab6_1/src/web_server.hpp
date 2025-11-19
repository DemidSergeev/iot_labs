#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <WebServer.h>
#include "microphone.hpp" // Now needed for plotter control

namespace web_server {

WebServer server(80);

// --- New Handlers for Plotter Control ---

void handlePlotStart() {
    mic::startPlotting();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}

void handlePlotStop() {
    mic::stopPlotting();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}


void handleRoot() {
    String html = "<html><head><title>ESP32 Sensor Hub</title></head><body>";
    html += "<h1>ESP32 Microphone Interface</h1>";
    html += "<h3>Recording</h3>";
    html += "<p><a href='/record?mock=0'>Record " + String(mic::RECORDING_DURATION_S) + " seconds of real audio.</a></p>";
    html += "<p><a href='/record?mock=1'>Generate " + String(mic::RECORDING_DURATION_S) + " seconds of mock audio.</a></p>";
    if (mic::getRecordingSize() > 0) {
        html += "<p><b>Recording available!</b> <a href='/download' download='recording.raw'>Download " + String(mic::getRecordingSize()) + "-byte file.</a></p>";
    }
    // --- New Links for Plotter ---
    html += "<hr><h3>Real-time Plotting</h3>";
    html += "<p>Use with Arduino IDE's Serial Plotter (baud: 115200).</p>";
    html += "<p><a href='/plot/start'>START Plotter</a></p>";
    html += "<p><a href='/plot/stop'>STOP Plotter</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// --- Other handlers (handleRecord, handleDownload, etc.) are unchanged ---

void handleRecord() {
    String mockParam = server.arg("mock");
    if (mockParam == "1") mic::generateMockData();
    else mic::recordToMemory();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting...");
}

/**
 * @brief Creates a 44-byte WAV file header in a buffer.
 * @param headerBuffer A pointer to a byte array of at least 44 bytes.
 * @param sampleRate The sample rate of the audio (e.g., 8000).
 * @param bitsPerSample The bit depth of the audio (e.g., 16).
 * @param dataSize The total size of the raw audio data in bytes.
 */
void createWavHeader(byte* headerBuffer, int sampleRate, int bitsPerSample, uint32_t dataSize) {
    // RIFF chunk descriptor
    headerBuffer[0] = 'R'; headerBuffer[1] = 'I'; headerBuffer[2] = 'F'; headerBuffer[3] = 'F';
    uint32_t chunkSize = dataSize + 36;
    headerBuffer[4] = (byte)(chunkSize & 0xFF);
    headerBuffer[5] = (byte)((chunkSize >> 8) & 0xFF);
    headerBuffer[6] = (byte)((chunkSize >> 16) & 0xFF);
    headerBuffer[7] = (byte)((chunkSize >> 24) & 0xFF);
    headerBuffer[8] = 'W'; headerBuffer[9] = 'A'; headerBuffer[10] = 'V'; headerBuffer[11] = 'E';

    // "fmt " sub-chunk
    headerBuffer[12] = 'f'; headerBuffer[13] = 'm'; headerBuffer[14] = 't'; headerBuffer[15] = ' ';
    headerBuffer[16] = 16; headerBuffer[17] = 0; headerBuffer[18] = 0; headerBuffer[19] = 0; // Sub-chunk size (16 for PCM)
    headerBuffer[20] = 1; headerBuffer[21] = 0; // Audio format (1 for PCM)
    headerBuffer[22] = 1; headerBuffer[23] = 0; // Number of channels (1 for mono)
    headerBuffer[24] = (byte)(sampleRate & 0xFF);
    headerBuffer[25] = (byte)((sampleRate >> 8) & 0xFF);
    headerBuffer[26] = 0; headerBuffer[27] = 0;
    uint32_t byteRate = sampleRate * 1 * (bitsPerSample / 8);
    headerBuffer[28] = (byte)(byteRate & 0xFF);
    headerBuffer[29] = (byte)((byteRate >> 8) & 0xFF);
    headerBuffer[30] = 0; headerBuffer[31] = 0;
    headerBuffer[32] = (bitsPerSample / 8); headerBuffer[33] = 0; // Block align
    headerBuffer[34] = bitsPerSample; headerBuffer[35] = 0; // Bits per sample

    // "data" sub-chunk
    headerBuffer[36] = 'd'; headerBuffer[37] = 'a'; headerBuffer[38] = 't'; headerBuffer[39] = 'a';
    headerBuffer[40] = (byte)(dataSize & 0xFF);
    headerBuffer[41] = (byte)((dataSize >> 8) & 0xFF);
    headerBuffer[42] = (byte)((dataSize >> 16) & 0xFF);
    headerBuffer[43] = (byte)((dataSize >> 24) & 0xFF);
}

/**
 * @brief Handles the download request and sends a complete .wav file.
 */
void handleDownload() {
    const uint8_t* buffer = mic::getRecordingBuffer();
    size_t size = mic::getRecordingSize();

    if (buffer && size > 0) {
        byte header[44];
        createWavHeader(header, mic::SAMPLE_RATE, mic::BITS_PER_SAMPLE, size);

        // Change the filename to .wav and set the correct total content length
        server.sendHeader("Content-Disposition", "attachment; filename=recording.wav");
        server.setContentLength(sizeof(header) + size);
        server.send(200, "audio/wav", ""); // Send headers

        // Send the file content in two parts: header then data
        server.sendContent((const char*)header, sizeof(header));
        server.sendContent((const char*)buffer, size);
        
    } else {
        server.send(404, "text/plain", "No recording found to download.");
    }
}

void handleNotFound() {
    server.send(404, "text/plain", "Not Found");
}

void setupServer() {
    Serial.println("[WEB] Initializing web server...");
    server.on("/", HTTP_GET, handleRoot);
    server.on("/record", HTTP_GET, handleRecord);
    server.on("/download", HTTP_GET, handleDownload);
    
    server.on("/plot/start", HTTP_GET, handlePlotStart);
    server.on("/plot/stop", HTTP_GET, handlePlotStop);

    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("[WEB] Web server started.");
}

void handleClient() {
    server.handleClient();
}

} // namespace web_server
#endif // WEB_SERVER_HPP