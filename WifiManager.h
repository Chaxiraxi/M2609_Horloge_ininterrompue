#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

#include "Notification.h"
#include "SECRETS.h"

class WiFiManager {
   public:
    explicit WiFiManager(Notification& notification);
    void connectToWiFi(char* SSID = WIFI_SSID, String PASSWORD = WIFI_PASSWORD);
    void disconnectWiFi();
    String sendGetRequest(const String& url);
    String sendPostRequest(const String& url, const String& body, const String& contentType = "application/json");

   private:
    Notification& notification;
    String extractHostFromUrl(const String& url);
    String extractPathFromUrl(const String& url);
};

#endif  // WIFI_MANAGER_H