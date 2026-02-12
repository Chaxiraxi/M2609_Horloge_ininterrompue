#include "WifiManager.h"

#include <WiFi.h>
#include <WiFiSSLClient.h>  // Replace with the correct WiFi library for your board
#include "Notification.h"
#include "SECRETS.h"  // Contains WIFI_SSID and WIFI_PASSWORD

WiFiManager::WiFiManager(Notification& notification) : notification(notification) {}

void WiFiManager::connectToWiFi(char* SSID, String PASSWORD) {
    WiFi.begin(SSID, PASSWORD.c_str());
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

String WiFiManager::extractHostFromUrl(const String& url) {
    int startIndex = url.indexOf("://");
    if (startIndex == -1) {
        startIndex = 0;
    } else {
        startIndex += 3;
    }
    int endIndex = url.indexOf("/", startIndex);
    if (endIndex == -1) {
        endIndex = url.length();
    }
    return url.substring(startIndex, endIndex);
}

String WiFiManager::extractPathFromUrl(const String& url) {
    int startIndex = url.indexOf("://");
    if (startIndex == -1) {
        startIndex = 0;
    } else {
        startIndex += 3;
    }
    int pathIndex = url.indexOf("/", startIndex);
    if (pathIndex == -1) {
        return "/";
    }
    return url.substring(pathIndex);
}

String WiFiManager::sendGetRequest(const String& url) {
    if (WiFi.status() != WL_CONNECTED) {
        notification.error("Not connected to WiFi. Cannot send GET request.");
        return "";
    }

    WiFiSSLClient client;

    String host = extractHostFromUrl(url);
    String path = extractPathFromUrl(url);

    notification.debug("Sending GET request to: " + url);
    bool connected = false;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        if (client.connect(host.c_str(), 443)) {
            notification.debug("Connection to server established on attempt " + String(attempt) + ".");
            connected = true;
            break;
        }
        notification.warning("Connection attempt " + String(attempt) + " failed.");
        delay(100);
    }
    if (!connected) {
        notification.error("Connection to server failed after 3 attempts.");
        return "";
    }

    client.println("GET " + path + " HTTP/1.1");
    client.println("Host: " + host);
    client.println("Connection: close");
    client.println();

    uint8_t tryCount = 0;
    while (!client.available() && tryCount < 20) {
        delay(10);
        tryCount++;
    }
    notification.debug("client.available() after wait: " + String(client.available()));

    String response;
    while (client.available()) {
        response += static_cast<char>(client.read());
    }
    client.stop();
    return response;
}

String WiFiManager::sendPostRequest(const String& url, const String& body, const String& contentType) {
    if (WiFi.status() != WL_CONNECTED) {
        notification.error("Not connected to WiFi. Cannot send POST request.");
        return "";
    }

    WiFiSSLClient client;

    String host = extractHostFromUrl(url);
    String path = extractPathFromUrl(url);

    client.connect(host.c_str(), 443);

    // Construct HTTP POST request
    client.println("POST " + path + " HTTP/1.1");
    client.println("Host: " + host);
    client.println("Content-Type: " + contentType);
    client.println("Content-Length: " + String(body.length()));
    client.println("Connection: close");
    client.println();
    client.println(body);

    char recievedBytesBuffer[512];
    uint32_t recievedBytes = 0;
    while (client.available() && recievedBytes < sizeof(recievedBytesBuffer) - 1) {
        char c = client.read();
        recievedBytes++;
    }
    recievedBytesBuffer[recievedBytes] = '\0';  // Null-terminate the buffer
    client.stop();
    return String(recievedBytesBuffer);
}