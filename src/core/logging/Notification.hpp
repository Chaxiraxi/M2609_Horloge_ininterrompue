/* Notification.h
   Modular notification system for Arduino sketches.
    - Supports multiple notification transports per notifier
    - Each transport has its own minimum notification level
    - Pluggable transport via NotificationTransport interface
*/
#pragma once

#include <Arduino.h>

/**
 * @brief Abstract transport interface for notifications.
 * @details
 * Description:
 *   Defines the contract used by Notification to forward messages to one or more output channels
 *   (e.g., Serial, LCD, web endpoint).
 *
 * Implement this interface to route notifications to another sink (HTTP,
 * file, display, etc.). Implementations should be lightweight and
 * non-blocking where possible for embedded use.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
class NotificationTransport {
   public:
    /**
     * @brief Virtual destructor to allow proper cleanup via base pointer.
     * @details
     * Description:
     *   Ensures derived transport implementations are destroyed correctly when handled via
     *   a NotificationTransport pointer.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    virtual ~NotificationTransport() {}

    /**
     * @brief Send a notification using this transport.
     * @details
     * Description:
     *   Called by Notification whenever a message passes the per-transport log-level filter.
     *
     * @param level Human-readable textual level (e.g. "INFO").
     * @param message Body of the notification.
     * @param timestamp Milliseconds elapsed since Notification initialization.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    virtual void send(const String& level, const String& message, unsigned long timestamp) = 0;
};

/**
 * @brief Notification helper with logging-like levels and pluggable transport.
 * @details
 * Description:
 *   Routes messages to multiple registered transports.
 *   Each transport has an independent minimum level filter.
 *
 * Usage examples:
 * - notifier.send(Notification::ERROR, "Something failed");
 * - notifier.info("Started");
 *
 * A Notification instance can route one message to several transports.
 * Every transport defines its own minimum log level threshold.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
class Notification {
   public:
    /**
     * @brief Notification severity levels.
     * @details
     * Description:
     *   Severity values used for message filtering and formatting.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    /**
     * @brief Construct an empty Notification router.
     * @details
     * Description:
     *   Initializes the internal transport registry and stores the start time used for timestamps.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    Notification();

    /**
     * @brief Destroy the Notification object.
     * @details
     * Description:
     *   Clears internal transport registrations. Transport objects remain owned by the caller.
     *
     * Notification never owns transports and therefore never deletes them.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    ~Notification();

    /**
     * @brief Add a transport with its own minimum level.
     * @details
     * Description:
     *   Registers one transport for fan-out output. Duplicate pointers are rejected.
     *
     * @param transport Pointer to a NotificationTransport implementation.
     * @param minLevel Minimum level that this transport should output.
     * @return true if transport was added, false on duplicate, null pointer, or full list.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    bool addTransport(NotificationTransport* transport, Level minLevel = INFO);

    /**
     * @brief Remove one transport from the notification fan-out list.
     * @details
     * Description:
     *   Unregisters the given transport pointer from Notification routing.
     *
     * @param transport Pointer previously registered with addTransport().
     * @return true if removed, false if not found.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    bool removeTransport(NotificationTransport* transport);

    /**
     * @brief Remove all transports from the notification fan-out list.
     * @details
     * Description:
     *   Clears all route registrations without deleting transport objects.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void clearTransports();

    /**
     * @brief Update the minimum level for one registered transport.
     * @details
     * Description:
     *   Changes filtering threshold for an already registered transport.
     *
     * @param transport Registered transport pointer.
     * @param minLevel New minimum level.
     * @return true if transport exists and was updated, false otherwise.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    bool setTransportLevel(NotificationTransport* transport, Level minLevel);

    /**
     * @brief Send a message at the given level.
     * @details
     * Description:
     *   Dispatches the message to every registered transport whose minimum level is
     *   lower than or equal to the message level.
     *
     * @param level Severity level from the Level enum.
     * @param message Message body to send.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void send(Level level, const String& message);

    /**
     * @brief Convenience helper: send a DEBUG-level notification.
     * @details
     * Description:
     *   Equivalent to send(DEBUG, message).
     *
     * @param message Message body to send.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void debug(const String& message);

    /**
     * @brief Convenience helper: send an INFO-level notification.
     * @details
     * Description:
     *   Equivalent to send(INFO, message).
     *
     * @param message Message body to send.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void info(const String& message);

    /**
     * @brief Convenience helper: send a WARNING-level notification.
     * @details
     * Description:
     *   Equivalent to send(WARNING, message).
     *
     * @param message Message body to send.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void warning(const String& message);

    /**
     * @brief Convenience helper: send an ERROR-level notification.
     * @details
     * Description:
     *   Equivalent to send(ERROR, message).
     *
     * @param message Message body to send.
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    void error(const String& message);

   private:
    static const uint8_t MAX_TRANSPORTS = 8;

    struct TransportBinding {
        NotificationTransport* transport;
        Level minLevel;
    };

    TransportBinding bindings_[MAX_TRANSPORTS];
    uint8_t bindingCount_;

    /**
     * @brief Convert the Level enum value to a human-readable String.
     * @details
     * Description:
     *   Produces a fixed-width textual label used by transports that print level names.
     *
     * @param level Enum value representing the log level.
     * @return String Textual name of the provided level ("DEBUG","INFO","WARNING","ERROR","UNKNOWN").
     *
     * @author GOLETTA David
     * @date 12/02/2026
     */
    String levelToString(Level level);

    unsigned long initTime_;
};
