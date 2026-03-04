#include "Notification.hpp"

#include <Arduino.h>

Notification::Notification()
    : bindingCount_(0), initTime_(millis()) {
}

Notification::~Notification() {
    clearTransports();
}

bool Notification::addTransport(NotificationTransport* transport, Notification::Level minLevel) {
    if (transport == nullptr) {
        return false;
    }

    for (uint8_t i = 0; i < bindingCount_; ++i) {
        if (bindings_[i].transport == transport) {
            return false;
        }
    }

    if (bindingCount_ >= MAX_TRANSPORTS) {
        return false;
    }

    bindings_[bindingCount_].transport = transport;
    bindings_[bindingCount_].minLevel = minLevel;
    ++bindingCount_;
    return true;
}

bool Notification::removeTransport(NotificationTransport* transport) {
    for (uint8_t i = 0; i < bindingCount_; ++i) {
        if (bindings_[i].transport == transport) {
            for (uint8_t j = i; j + 1 < bindingCount_; ++j) {
                bindings_[j] = bindings_[j + 1];
            }
            --bindingCount_;
            return true;
        }
    }

    return false;
}

void Notification::clearTransports() {
    bindingCount_ = 0;
}

bool Notification::setTransportLevel(NotificationTransport* transport, Notification::Level minLevel) {
    for (uint8_t i = 0; i < bindingCount_; ++i) {
        if (bindings_[i].transport == transport) {
            bindings_[i].minLevel = minLevel;
            return true;
        }
    }

    return false;
}

/**
 * @internal
 * @brief Convert a notification level enum to fixed-width text.
 * @details
 * Produces stable textual labels used by transports to render level prefixes.
 *
 * @param level Notification level value.
 * @return String label associated with the given level.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
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
    const String levelText = levelToString(level);
    const unsigned long timestamp = millis() - initTime_;

    for (uint8_t i = 0; i < bindingCount_; ++i) {
        if (bindings_[i].transport != nullptr && level >= bindings_[i].minLevel) {
            bindings_[i].transport->send(levelText, message, timestamp);
        }
    }
}

void Notification::debug(const String& message) { send(DEBUG, message); }
void Notification::info(const String& message) { send(INFO, message); }
void Notification::warning(const String& message) { send(WARNING, message); }
void Notification::error(const String& message) { send(ERROR, message); }
