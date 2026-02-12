#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

#include "Notification.h"
#include "SECRETS.h"

/**
 * @brief Utility class for managing Wi-Fi connectivity.
 * @details
 * Description:
 *   Wraps connection and disconnection logic for the board Wi-Fi interface and emits
 *   progress/status messages through the provided Notification instance.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
class WiFiManager {
   public:
    /**
     * @brief Construct a WiFiManager bound to a Notification router.
     * @details
     * Description:
     *   Stores a reference to Notification used for debug/info/warning/error messages.
     *
     * @param notification Notification instance used for status reporting.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    explicit WiFiManager(Notification& notification);

    /**
     * @brief Start and monitor connection to a Wi-Fi network.
     * @details
     * Description:
     *   Attempts to connect to the provided SSID/password, emits periodic logs while waiting,
     *   and reports success, failure, and firmware-version status.
     *
     * @param SSID SSID name of the Wi-Fi network.
     * @param PASSWORD Password for the Wi-Fi network.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void connectToWiFi(char* SSID = WIFI_SSID, String PASSWORD = WIFI_PASSWORD);

    /**
     * @brief Disconnect from the current Wi-Fi network.
     * @details
     * Description:
     *   Requests Wi-Fi disconnection and emits an informational notification.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void disconnectWiFi();

   private:
    Notification& notification;
};

#endif  // WIFI_MANAGER_H