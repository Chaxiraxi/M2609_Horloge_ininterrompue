#include "utils.h"

void printDabTime(DABTime& dabtime) {
    Serial.print(F("Local Time (DAB): "));
    Serial.print(dabtime.Days);
    Serial.print('/');
    Serial.print(dabtime.Months);
    Serial.print('/');
    Serial.print(dabtime.Year);
    Serial.print(' ');
    if (dabtime.Hours < 10) {
        Serial.print('0');
    }
    Serial.print(dabtime.Hours);
    Serial.print(':');
    if (dabtime.Minutes < 10) {
        Serial.print('0');
    }
    Serial.print(dabtime.Minutes);
    Serial.print(':');
    if (dabtime.Seconds < 10) {
        Serial.print('0');
    }
    Serial.println(dabtime.Seconds);
}

void printGpsTime(Adafruit_GPS& GPS) {
    Serial.print("\nUTC Time (GPS): ");
    Serial.print(GPS.day);
    Serial.print('/');
    Serial.print(GPS.month);
    Serial.print('/');
    Serial.print(GPS.year);
    Serial.print(' ');

    if (GPS.hour < 10) {
        Serial.print('0');
    }
    Serial.print(GPS.hour, DEC);
    Serial.print(':');
    if (GPS.minute < 10) {
        Serial.print('0');
    }
    Serial.print(GPS.minute, DEC);
    Serial.print(':');
    if (GPS.seconds < 10) {
        Serial.print('0');
    }
    Serial.print(GPS.seconds, DEC);
    Serial.print('.');
    if (GPS.milliseconds < 10) {
        Serial.print("00");
    } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
        Serial.print("0");
    }
    Serial.println(GPS.milliseconds);
}

// clang-format off
String dateToDayOfTheWeek(uint16_t year, uint8_t month, uint8_t day, bool shortForm) {
    // Zeller's Congruence algorithm
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    uint16_t k = year % 100;
    uint16_t j = year / 100;
    uint8_t h = (day + (13 * (month + 1)) / 5 + k + (k / 4) + (j / 4) - (2 * j)) % 7;

    switch (h) {
        case 0: return shortForm ? "Sat" : "Saturday";
        case 1: return shortForm ? "Sun" : "Sunday";
        case 2: return shortForm ? "Mon" : "Monday";
        case 3: return shortForm ? "Tue" : "Tuesday";
        case 4: return shortForm ? "Wed" : "Wednesday";
        case 5: return shortForm ? "Thu" : "Thursday";
        case 6: return shortForm ? "Fri" : "Friday";
        default: return "";
    }
}
// clang-format on

String formatTwoDigits(uint8_t value) {
    return (value < 10 ? "0" : "") + String(value);
}