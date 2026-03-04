#include "SerialTransport.hpp"

#include <Arduino.h>

SerialTransport::SerialTransport(unsigned long baud)
    : baud_(baud) {
}

SerialTransport::~SerialTransport() {
    Serial.end();
}

void SerialTransport::init() {
    Serial.begin(baud_);
}

void SerialTransport::end() {
    Serial.end();
}

void SerialTransport::send(const String& level, const String& message, unsigned long timestamp) {
    unsigned long seconds = timestamp / 1000;
    unsigned long milliseconds = timestamp % 1000;
    unsigned long minutes = seconds / 60;
    seconds %= 60;
    unsigned long hours = minutes / 60;
    minutes %= 60;

    String timeString = String(hours) + ":" +
                        (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                        (seconds < 10 ? "0" : "") + String(seconds) + "." +
                        (milliseconds < 100 ? (milliseconds < 10 ? "00" : "0") : "") + String(milliseconds);

    Serial.println("[" + timeString + "] [" + level + "] " + message);
}
