#ifndef NTP_HPP
#define NTP_HPP

#include <Arduino.h>
#include <time.h>

namespace ntp {

const char* server = "pool.ntp.org";
// Moscow Timezone: MSK (UTC+3)
// POSIX Format: "MSK-3" 
const char* timeZone = "MSK-3"; 

void init() {
    Serial.println("[NTP] Setting up time...");
    // Config time with specific timezone string
    configTzTime(timeZone, server);
}

bool isTimeSet() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    // Simple check: if year is less than 2020, time hasn't synced
    return (timeinfo.tm_year + 1900) > 2020;
}

struct tm getCurrentTime() {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    return timeinfo;
}

} // namespace ntp
#endif // NTP_HPP