#pragma once

#include <Arduino.h>

#include "Notification.hpp"

/**
 * @brief A simple transport that writes notifications to the Arduino Serial.
 * @details
 * Description:
 *   Outputs formatted messages including timestamp and level to the Serial interface.
 *
 * It writes a bracketed level followed by the message and a
 * newline, e.g. "[INFO] Hardware initialized".
 *
 * @author GOLETTA David
 * @date 04/03/2026
 */
class SerialTransport : public NotificationTransport {
   public:
    /**
     * @brief Construct a new SerialTransport object.
     * @details
     * Description:
     *   Stores the baud rate used when init() is called.
     *
     * @param baud Baud rate for Serial communication (default 9600).
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    explicit SerialTransport(unsigned long baud = 9600);

    /**
     * @brief Destroy the SerialTransport object.
     * @details
     * Description:
     *   Allows transport-specific cleanup to run.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    ~SerialTransport() override;

    /**
     * @brief Initialize Serial communication.
     * @details
     * Description:
     *   Starts the Serial interface using the configured baud rate.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    void init();

    /**
     * @brief End Serial communication.
     * @details
     * Description:
     *   Stops the Serial interface.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    void end();

    /**
     * @brief Send a formatted message to Serial.
     * @details
     * Description:
     *   Builds a human-readable line with elapsed time, level, and message body, then writes it to Serial.
     *
     * @param level Textual representation of the log level (e.g. "INFO").
     * @param message Body of the notification to send.
     * @param timestamp Milliseconds elapsed since Notification initialization.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    void send(const String& level, const String& message, unsigned long timestamp) override;

   private:
    unsigned long baud_;
};
