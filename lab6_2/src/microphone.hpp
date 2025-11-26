#ifndef MICROPHONE_HPP
#define MICROPHONE_HPP

#include <Arduino.h>
#include "driver/i2s.h"

namespace mic {

// --- Configuration ---
constexpr i2s_port_t I2S_PORT = I2S_NUM_0;
constexpr adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_6; // GPIO 34
constexpr int SAMPLE_RATE = 44100;
constexpr i2s_bits_per_sample_t BITS_PER_SAMPLE = I2S_BITS_PER_SAMPLE_16BIT;
constexpr int NUM_CHANNELS = 1;

/**
 * @brief Configures the I2S peripheral to read from the built-in ADC.
 * @return true on success, false on failure.
 */
bool setupMic() {
    Serial.println("[MIC] Initializing microphone...");
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
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
    if (i2s_adc_enable(I2S_PORT) != ESP_OK) {
        Serial.println("[MIC] Error enabling ADC for I2S.");
        return false;
    }
    Serial.println("[MIC] Microphone initialized successfully.");
    return true;
}

/**
 * @brief Reads a chunk of audio data from the I2S peripheral.
 * @param buffer A pointer to the buffer where data should be stored.
 * @param requested_size The maximum number of bytes to read.
 * @return The number of bytes actually read.
 */
size_t readChunk(uint8_t* buffer, size_t buffer_size) {
    size_t bytes_read = 0;
    // Create a temporary stereo buffer on the stack, twice the size of the destination buffer.
    size_t stereo_buffer_size = buffer_size * 2;
    uint16_t stereo_buffer[stereo_buffer_size];

    size_t bytes_read_stereo = 0;
    i2s_read(I2S_PORT, stereo_buffer, stereo_buffer_size, &bytes_read_stereo, pdMS_TO_TICKS(100));
    bytes_read = bytes_read_stereo / 2;

    uint16_t* mono_buffer = reinterpret_cast<uint16_t*>(buffer);

    // Down-mix by taking every other sample (the left channel)
    int stereo_sample_count = bytes_read_stereo / sizeof(uint16_t);
    for (int i = 0; i < stereo_sample_count; i += 2) {
        mono_buffer[i / 2] = stereo_buffer[i];
    }

    int16_t* int16_buffer = reinterpret_cast<int16_t*>(buffer);
    // Convert 16-bit samples to 12-bit by masking and centering around 0
    for (size_t i = 0; i < bytes_read / sizeof(int16_t); ++i) {
        int16_buffer[i] = int16_buffer[i] & 0x0FFF - 2048;
    }

    return bytes_read;
}

} // namespace mic
#endif // MICROPHONE_HPP