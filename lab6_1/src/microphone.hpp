#ifndef MICROPHONE_HPP
#define MICROPHONE_HPP

#include <Arduino.h>
#include "driver/i2s.h"

namespace mic {

// --- Configuration ---
constexpr i2s_port_t I2S_PORT = I2S_NUM_0;
constexpr adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_6; // GPIO 34
constexpr int SAMPLE_RATE = 8000;
constexpr i2s_bits_per_sample_t BITS_PER_SAMPLE = I2S_BITS_PER_SAMPLE_16BIT;

// --- CORRECTED: Acknowledge the hardware's 2-channel output ---
constexpr int NUM_CHANNELS = 2; 

// --- In-Memory Recording Buffer ---
constexpr int RAM_REC_DURATION_S = 2;
// --- CORRECTED: Buffer size now correctly accounts for 2 channels ---
constexpr int RAM_BUFFER_SIZE = RAM_REC_DURATION_S * SAMPLE_RATE * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

static uint8_t ram_recording_buffer[RAM_BUFFER_SIZE];
size_t ram_data_size = 0;
volatile bool is_plotting = false;

bool setupMic() {
    Serial.println("[MIC] Initializing microphone...");
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // This is required for ADC mode
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8, .dma_buf_len = 256, .use_apll = false, .tx_desc_auto_clear = false, .fixed_mclk = 0
    };
    if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) { return false; }
    if (i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL) != ESP_OK) { return false; }
    if (i2s_adc_enable(I2S_PORT) != ESP_OK) { return false; }
    Serial.println("[MIC] Microphone initialized successfully.");
    return true;
}

// --- Plotting Functions ---
void startPlotting() { is_plotting = true; Serial.println("[MIC] Plotter enabled."); }
void stopPlotting() { is_plotting = false; Serial.println("[MIC] Plotter disabled."); }

/**
 * @brief Reads a full stereo frame and prints only the left channel.
 */
void readAndPrintSignal() {
    if (!is_plotting) {
        return;
    }
    uint16_t sample_buffer[2]; // Buffer for one full stereo frame (L+R = 4 bytes)
    size_t bytes_read = 0;
    i2s_read(I2S_PORT, sample_buffer, sizeof(sample_buffer), &bytes_read, pdMS_TO_TICKS(100));

    if (bytes_read > 0) {
        // Print the 12 bits of left channel sample
        Serial.print(">signal:");
        Serial.println(sample_buffer[0] & 0x0FFF);
    }
}

// --- In-Memory Recording Functions ---
bool recordToMemory() {
    Serial.printf("[MIC] Recording to RAM for %d seconds...\n", RAM_REC_DURATION_S);
    ram_data_size = 0;
    size_t bytes_read = 0;
    i2s_read(I2S_PORT, ram_recording_buffer, RAM_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    if (bytes_read != RAM_BUFFER_SIZE) {
        Serial.printf("[MIC] Recording error. Read %u, expected %u\n", bytes_read, RAM_BUFFER_SIZE);
        return false;
    }
    ram_data_size = bytes_read;
    Serial.println("[MIC] RAM recording finished.");
    return true;
}

/**
 * @brief CORRECTED: Generates a duplicated stereo signal for consistency.
 */
void generateMockData() {
    Serial.println("[MIC] Generating mock stereo data...");
    int16_t* samples = (int16_t*)ram_recording_buffer;
    int num_stereo_samples = RAM_BUFFER_SIZE / sizeof(int16_t);
    for (int i = 0; i < num_stereo_samples; i += 2) {
        int16_t sample_value = (int16_t)(10000.0 * sin(2.0 * PI * 440.0 * ((float)(i/2) / SAMPLE_RATE)));
        samples[i] = sample_value;     // Left channel
        samples[i + 1] = sample_value; // Right channel
    }
    ram_data_size = RAM_BUFFER_SIZE;
}

const uint8_t* getRamBuffer() { return ram_recording_buffer; }
size_t getRamBufferSize() { return ram_data_size; }

} // namespace mic
#endif // MICROPHONE_HPP