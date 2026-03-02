#include "WifiManager.hpp"

#include <WiFiS3.h>

#include "../core/logging/Notification.hpp"

WiFiManager::WiFiManager(Notification& notification) : notification(notification) {}

namespace {
/**
 * @internal
 * @brief Convert an IPAddress to dotted-decimal text.
 * @details
 * Formats the four IP octets into a human-readable IPv4 string.
 *
 * @param ip IP address to format.
 * @return Dotted-decimal representation of the address.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
String ipToString(const IPAddress& ip) {
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}
}  // namespace

void WiFiManager::connectToWiFi(const char* SSID, const char* PASSWORD) {
    WiFi.begin(SSID, PASSWORD);
    notification.debug("Connecting to WiFi SSID: " + String(SSID));
    const unsigned long connectionTimeout = 20000;
    const unsigned long logInterval = 500;
    unsigned long startAttemptTime = millis();
    int maxAttempts = connectionTimeout / logInterval;
    if (maxAttempts == 0) {
        maxAttempts = 1;
    }
    int attempts = 0;
    unsigned long lastLogTime = 0;

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < connectionTimeout && attempts < maxAttempts) {
        unsigned long now = millis();
        if (now - lastLogTime >= logInterval) {
            attempts++;
            notification.debug("Attempting to connect to WiFi... (" + String(attempts) + "/" + String(maxAttempts) + ")");
            lastLogTime = now;
        }
        yield();
    }

    if (WiFi.status() == WL_CONNECTED) {
        notification.info("Connected to WiFi!");
        String firmwareVersion = WiFi.firmwareVersion();
        notification.debug("WiFi Firmware Version: " + firmwareVersion);
        if (firmwareVersion < WIFI_FIRMWARE_LATEST_VERSION) {
            notification.warning("WiFi firmware is outdated. Consider updating to the latest version. (Current version: " + firmwareVersion + ", Latest version: " + WIFI_FIRMWARE_LATEST_VERSION + ")");
        }
    } else {
        notification.error("Failed to connect to WiFi. Starting fallback open access point.");
        startFallbackAccessPoint();
    }
}

/**
 * @internal
 * @brief Start fallback access-point mode.
 * @details
 * Disconnects station mode and attempts to open a local AP for recovery setup.
 * Emits status messages describing AP startup success or failure.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void WiFiManager::startFallbackAccessPoint() {
    const char* fallbackSsid = "DabGps-Setup";

    WiFi.disconnect();
    int apStatus = WiFi.beginAP(fallbackSsid);

    if (apStatus == WL_AP_LISTENING || apStatus == WL_AP_CONNECTED) {
        notification.info("Fallback AP started. SSID: " + String(fallbackSsid));
        notification.info("Connect to AP and reach device at " + ipToString(WiFi.localIP()));
    } else {
        notification.error("Failed to start fallback AP.");
    }
}

void WiFiManager::disconnectWiFi() {
    WiFi.disconnect();
    notification.info("Disconnected from WiFi.");
}