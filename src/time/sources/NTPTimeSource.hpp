#pragma once

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "../../core/logging/Notification.hpp"
#include "../core/TimeSource.hpp"

/**
 * @brief Time source implementation backed by an NTP server.
 * @details
 * Description:
 *   Uses NTPClient to retrieve the current time via Wi-Fi and converts it to DateTimeFields.
 *   A timezone offset is applied via NTPClient so date changes are handled correctly.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
class NTPTimeSource : public TimeSource {
   public:
    /**
     * @brief Construct an NTPTimeSource.
     * @details
     * Description:
     *   Creates an NTP client targeting the given server with a timezone offset in seconds.
     *
     * @param server NTP server hostname (e.g., "pool.ntp.org").
     * @param timezoneOffsetSeconds Timezone offset in seconds (e.g., +3600 for CET).
     * @param notifier Optional notifier for status messages.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    NTPTimeSource(const char* server, int32_t timezoneOffsetSeconds = 0, Notification* notifier = nullptr);

    /**
     * @brief Initialize the NTP client.
     * @details
     * Description:
     *   Starts the UDP client and prepares NTP updates.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void init();

    /**
     * @brief Get the current date and time from NTP.
     * @details
     * Description:
     *   Attempts to update the NTP client and fills out when valid epoch time is available.
     *
     * @param out Reference to a DateTimeFields structure to be filled.
     * @return True if out contains valid NTP-derived values; false otherwise.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    bool getDateTime(DateTimeFields& out) override;

   private:
    bool isWiFiConnected() const;
    bool epochToDateTime(uint32_t epochSeconds, DateTimeFields& out) const;

    WiFiUDP udp_;
    NTPClient timeClient_;
    Notification* notifier_;
};
