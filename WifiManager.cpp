#include "WifiManager.h"

#include <WiFiS3.h>

#include "Notification.h"

WiFiManager::WiFiManager(Notification& notification) : notification(notification) {}

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
        notification.error("Failed to connect to WiFi.");
    }
}

void WiFiManager::disconnectWiFi() {
    WiFi.disconnect();
    notification.info("Disconnected from WiFi.");
}