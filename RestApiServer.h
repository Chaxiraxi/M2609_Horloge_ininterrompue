#pragma once

#include <Arduino.h>
#include <WiFiS3.h>

#include "Notification.h"
#include "TimeCoordinator.h"
#include "TimeSource.h"

/**
 * @brief Lightweight HTTP REST server for remote clock control.
 * @details
 * Description:
 *   Exposes a minimal API and a simple embedded web page to:
 *   - Enable/disable DAB, NTP, GPS sources.
 *   - Set manual date-time.
 *   - Read current status.
 */
class RestApiServer {
   public:
    RestApiServer(TimeCoordinator& coordinator,
                  TimeSource* sources[], uint8_t sourceCount,
                  Notification* notifier = nullptr,
                  uint16_t port = 80);

    void begin();
    void update();

   private:
    bool sourceIndexFromName(const String& sourceName, uint8_t& outIndex) const;
    bool parseRequestLine(const String& requestLine, String& method, String& path) const;

    String readBody(WiFiClient& client, int contentLength) const;
    String urlDecode(const String& input) const;
    String getParam(const String& body, const String& key) const;

    void sendResponse(WiFiClient& client, int code, const String& contentType, const String& body) const;
    void sendStatus(WiFiClient& client) const;
    void sendWebPage(WiFiClient& client) const;

    bool handleToggleSource(const String& body, String& message);
    bool handleSetTime(const String& body, String& message);

    String currentTimeIsoString(bool& isSet) const;

    TimeCoordinator& coordinator_;
    TimeSource* sources_[TimeCoordinator::MAX_SOURCES] = {};
    uint8_t sourceCount_ = 0;
    bool manualTimeSet_ = false;
    Notification* notifier_ = nullptr;
    WiFiServer server_;
};
