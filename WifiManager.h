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

   private:
    Notification& notification;
};

#endif  // WIFI_MANAGER_H