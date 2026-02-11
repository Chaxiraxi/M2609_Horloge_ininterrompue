#include "GPSTimeSource.h"

GPSTimeSource::GPSTimeSource(Adafruit_GPS& gps, int8_t timezoneOffsetHours)
    : gps_(gps), timezoneOffsetHours_(timezoneOffsetHours) {}

void GPSTimeSource::init() {
    // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
    gps_.begin(9600);

    // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
    gps_.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // uncomment this line to turn on only the "minimum recommended" data
    // gps_.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
    // the parser doesn't care about other sentences at this time

    // Set the update rate
    gps_.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);  // 1 Hz update rate
    // For the parsing code to work nicely and have time to sort thru the data, and
    // print it out we don't suggest using anything higher than 1 Hz

    // Request updates on antenna status, comment out to keep quiet
    gps_.sendCommand(PGCMD_ANTENNA);

    delay(1000);
    // Ask for firmware version
    gps_.sendCommand(PMTK_Q_RELEASE);
}

bool GPSTimeSource::isSaneDateTime() const {
    if (gps_.year == 0 || gps_.month == 0 || gps_.day == 0) {
        return false;
    }
    if (gps_.month > 12 || gps_.day > 31) {
        return false;
    }
    if (gps_.hour > 23 || gps_.minute > 59 || gps_.seconds > 59) {
        return false;
    }
    return true;
}

bool GPSTimeSource::getDateTime(DateTimeFields& out) {
    if (!isEnabled()) {
        return false;
    }
    if (!isSaneDateTime()) {
        return false;
    }

    out.date.year = static_cast<uint16_t>(2000 + gps_.year);
    out.date.month = gps_.month;
    out.date.day = gps_.day;

    out.time.hour = wrapHours(static_cast<int32_t>(gps_.hour) + timezoneOffsetHours_);
    out.time.minute = gps_.minute;
    out.time.second = gps_.seconds;

    return true;
}
