#include "NTPTimeSource.h"

#if defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#else
#include <WiFi.h>
#endif

namespace {
bool isLeapYear(uint16_t year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return (year % 4 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
    static const uint8_t kDaysPerMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return kDaysPerMonth[month - 1];
}
}  // namespace

NTPTimeSource::NTPTimeSource(const char* server, int32_t timezoneOffsetSeconds, Notification* notifier)
    : timeClient_(udp_, server, timezoneOffsetSeconds), notifier_(notifier) {}

void NTPTimeSource::init() {
    timeClient_.begin();
    if (notifier_) {
        notifier_->info("NTP client started");
    }
}

bool NTPTimeSource::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool NTPTimeSource::epochToDateTime(uint32_t epochSeconds, DateTimeFields& out) const {
    if (epochSeconds == 0) return false;

    uint32_t secondsOfDay = epochSeconds % 86400UL;
    uint32_t days = epochSeconds / 86400UL;

    out.time.hour = static_cast<uint8_t>(secondsOfDay / 3600UL);
    secondsOfDay %= 3600UL;
    out.time.minute = static_cast<uint8_t>(secondsOfDay / 60UL);
    out.time.second = static_cast<uint8_t>(secondsOfDay % 60UL);

    uint16_t year = 1970;
    while (true) {
        uint16_t daysThisYear = static_cast<uint16_t>(isLeapYear(year) ? 366 : 365);
        if (days < daysThisYear) break;
        days -= daysThisYear;
        ++year;
    }

    uint8_t month = 1;
    while (true) {
        uint8_t dim = daysInMonth(year, month);
        if (days < dim) break;
        days -= dim;
        ++month;
    }

    out.date.year = year;
    out.date.month = month;
    out.date.day = static_cast<uint8_t>(days + 1);

    return true;
}

bool NTPTimeSource::getDateTime(DateTimeFields& out) {
    if (!isEnabled()) return false;
    if (!isWiFiConnected()) return false;

    timeClient_.update();
    uint32_t epochSeconds = timeClient_.getEpochTime();
    if (epochSeconds == 0) {
        if (notifier_) {
            notifier_->warning("NTP time not available yet");
        }
        return false;
    }

    return epochToDateTime(epochSeconds, out);
}
