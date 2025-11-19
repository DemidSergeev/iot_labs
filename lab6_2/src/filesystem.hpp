#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "FS.h"
#include <LittleFS.h>

namespace fs {

// A handle to the currently open file for writing
File current_write_file;

/**
 * @brief Initializes the LittleFS filesystem.
 */
bool init() {
    Serial.println("[FS] Initializing LittleFS...");
    if (!LittleFS.begin()) {
        Serial.println("[FS] An error occurred while mounting LittleFS.");
        return false;
    }
    Serial.println("[FS] LittleFS mounted successfully.");
    return true;
}

/**
 * @brief Opens a file for writing. Fails if a file is already open.
 * @param filename The full path of the file to open.
 * @return true on success, false on failure.
 */
bool openFileForWrite(const String& filename) {
    if (current_write_file) {
        Serial.println("[FS] Error: A file is already open for writing.");
        return false;
    }
    current_write_file = LittleFS.open(filename, FILE_WRITE);
    if (!current_write_file) {
        Serial.printf("[FS] Failed to open file: %s\n", filename.c_str());
        return false;
    }
    Serial.printf("[FS] Opened %s for writing.\n", filename.c_str());
    return true;
}

/**
 * @brief Appends a chunk of data to the currently open file.
 * @param data The buffer containing the data.
 * @param len The length of the data in bytes.
 * @return true on success, false on failure.
 */
bool appendChunk(const uint8_t* data, size_t len) {
    if (!current_write_file) {
        return false;
    }
    return current_write_file.write(data, len) == len;
}

/**
 * @brief Closes the currently open file.
 */
void closeFile() {
    if (current_write_file) {
        current_write_file.close();
        Serial.println("[FS] File closed.");
    }
}

/**
 * @brief Deletes a file from the filesystem.
 */
bool deleteFile(const String& filename) {
    Serial.printf("[FS] Deleting file: %s\n", filename.c_str());
    return LittleFS.remove(filename);
}

/**
 * @brief Generates an HTML list of files in the filesystem.
 */
String getFilesAsHTML() {
    String html = "<ul>";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    int count = 0;
    while (file) {
        if (!file.isDirectory()) {
            count++;
            String filename = String(file.path());
            html += "<li>" + filename + " (" + String(file.size()) + " bytes)";
            html += " | <a href='/download?file=" + filename + "'>Download WAV</a>";
            html += " | <a href='/delete?file=" + filename + "'>Delete</a>";
            html += "</li>";
        }
        file = root.openNextFile();
    }
    if (count == 0) {
        html += "<li>No recordings found.</li>";
    }
    html += "</ul>";
    return html;
}

} // namespace fs
#endif // FILESYSTEM_HPP