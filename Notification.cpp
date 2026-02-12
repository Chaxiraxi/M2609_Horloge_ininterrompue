#include "Notification.h"

#include <Arduino.h>

SerialTransport::SerialTransport(unsigned long baud)
    : _baud(baud) {
}

void SerialTransport::init() {
    Serial.begin(_baud);
}

SerialTransport::~SerialTransport() {
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

Notification::Notification(Notification::Level level, NotificationTransport* transport)
    : transport_(nullptr), ownsTransport_(false), currentLevel_(level), initTime_(millis()) {
    if (transport) {
        setTransport(transport, false);
    } else {
        setTransport(new SerialTransport(), true);
    }
}

Notification::~Notification() {
    if (ownsTransport_ && transport_) {
        delete transport_;
        transport_ = nullptr;
    }
}

void Notification::setTransport(NotificationTransport* transport, bool takeOwnership) {
    if (ownsTransport_ && transport_) {
        delete transport_;
    }
    transport_ = transport;
    ownsTransport_ = takeOwnership;
}

void Notification::setLevel(Notification::Level level) {
    currentLevel_ = level;
}

Notification::Level Notification::getLevel() const {
    return currentLevel_;
}

// clang-format off
String Notification::levelToString(Notification::Level level) {
    switch (level) {
        case DEBUG:     return "DEBUG  ";
        case INFO:      return "INFO   ";
        case WARNING:   return "WARNING";
        case ERROR:     return "ERROR  ";
        default:        return "UNKNOWN";
    }
}
// clang-format on

void Notification::send(Notification::Level level, const String& message) {
    // drop messages below the configured level
    if (level < currentLevel_) return;

    if (transport_) {
        transport_->send(levelToString(level), message, millis() - initTime_);
    }
}

void Notification::debug(const String& message) { send(DEBUG, message); }
void Notification::info(const String& message) { send(INFO, message); }
void Notification::warning(const String& message) { send(WARNING, message); }
void Notification::error(const String& message) { send(ERROR, message); }
