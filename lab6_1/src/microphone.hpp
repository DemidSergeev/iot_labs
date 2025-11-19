#ifndef MICROPHONE_HPP
#define MICROPHONE_HPP

#include <Arduino.h>
#include "driver/i2s.h"

namespace mic {

// --- Configuration ---
const i2s_port_t I2S_PORT = I2S_NUM_0;
const adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_6; // GPIO 34
const int SAMPLE_RATE = 8000;
const i2s_bits_per_sample_t BITS_PER_SAMPLE = I2S_BITS_PER_SAMPLE_16BIT;
const int RECORDING_DURATION_S = 3;
const int BUFFER_SIZE_BYTES = RECORDING_DURATION_S * SAMPLE_RATE * (BITS_PER_SAMPLE / 8);

static uint8_t recording_buffer[BUFFER_SIZE_BYTES];
size_t valid_data_size = 0;

// This flag controls the real-time plotting task
volatile bool is_plotting = false;

// --- Plotting Control Functions ---
/**
 * @brief Enables the real-time serial plotter output.
 */
void startPlotting() {
    Serial.println("[MIC] Starting real-time plotter.");
    is_plotting = true;
}

/**
 * @brief Disables the real-time serial plotter output.
 */
void stopPlotting() {
    Serial.println("[MIC] Stopping real-time plotter.");
    is_plotting = false;
}

/**
 * @brief If plotting is enabled, reads a single sample and prints it.
 * This is designed to be called continuously in a tight loop by an RTOS task.
 */
void readAndPrintSignal() {
    if (!is_plotting) {
        return; // Do nothing if plotting is disabled
    }
    
    uint16_t sample = 0;
    size_t bytes_read = 0;
    
    // Read one 16-bit sample, with a timeout to prevent blocking forever
    esp_err_t result = i2s_read(I2S_PORT, &sample, sizeof(uint16_t), &bytes_read, pdMS_TO_TICKS(100));

    if (result == ESP_OK && bytes_read > 0) {
        // The raw ADC value from ESP32 I2S is inverted and needs to be corrected.
        // It's a 12-bit value (0-4095) inside the 16-bit sample.
        Serial.println(4095 - sample);
    }
}

bool setupMic() {
    Serial.println("[MIC] Initializing microphone...");
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("[MIC] Error installing I2S driver.");
        return false;
    }
    if (i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL) != ESP_OK) {
        Serial.println("[MIC] Error setting up ADC for I2S.");
        return false;
    }
    Serial.println("[MIC] Microphone initialized successfully.");
    return true;
}

bool recordToMemory() {
    Serial.printf("[MIC] Starting recording for %d seconds...\n", RECORDING_DURATION_S);
    valid_data_size = 0;
    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_PORT, recording_buffer, BUFFER_SIZE_BYTES, &bytes_read, portMAX_DELAY);
    if (result != ESP_OK || bytes_read != BUFFER_SIZE_BYTES) {
        Serial.printf("[MIC] Recording error. Read %d bytes, expected %d. Error: %d\n", bytes_read, BUFFER_SIZE_BYTES, result);
        return false;
    }
    valid_data_size = BUFFER_SIZE_BYTES;
    Serial.println("[MIC] Recording finished.");
    return true;
}

void generateMockData() {
    Serial.println("[MIC] Generating mock data...");
    int16_t* samples = (int16_t*)recording_buffer;
    int num_samples = BUFFER_SIZE_BYTES / 2;
    for (int i = 0; i < num_samples; i++) {
        samples[i] = (int16_t)(10000.0 * sin(2.0 * PI * 440.0 * ((float)i / SAMPLE_RATE)));
    }
    valid_data_size = BUFFER_SIZE_BYTES;
    Serial.printf("[MIC] Generated %d bytes of mock data.\n", BUFFER_SIZE_BYTES);
}

const uint8_t* getRecordingBuffer() { return recording_buffer; }
size_t getRecordingSize() { return valid_data_size; }

} // namespace mic
#endif // MICROPHONE_HPP