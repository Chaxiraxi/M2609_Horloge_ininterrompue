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
    /**
     * @brief Construct the REST API server.
     * @details
     * Description:
     *   Stores references to the coordinator and source list, configures the HTTP listening port,
     *   and optionally enables notification logging.
     *
     * @param coordinator Reference to the time coordinator used by API handlers.
     * @param sources Array of pointers to time sources.
     * @param sourceCount Number of entries in the sources array.
     * @param notifier Optional notifier used for request/status logging.
     * @param port TCP port used by the embedded HTTP server.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    RestApiServer(TimeCoordinator& coordinator,
                  TimeSource* sources[], uint8_t sourceCount,
                  Notification* notifier = nullptr,
                  uint16_t port = 80);

    /**
     * @brief Start the embedded HTTP server.
     * @details
     * Description:
     *   Initializes the underlying WiFiServer so incoming client connections can be accepted.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void begin();

    /**
     * @brief Process pending HTTP clients and requests.
     * @details
     * Description:
     *   Polls the server, reads requests, dispatches handlers, and writes responses.
     *   This method is intended to be called regularly from loop().
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
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
