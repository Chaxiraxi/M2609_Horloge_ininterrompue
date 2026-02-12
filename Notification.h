/* Notification.h
   Modular notification system for Arduino sketches.
   - Constructor starts Serial communication (default 9600)
   - Supports different notification levels
   - Pluggable transport via NotificationTransport interface
*/
#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <Arduino.h>

/**
 * @brief Abstract transport interface for notifications.
 *
 * Implement this interface to route notifications to another sink (HTTP,
 * file, display, etc.). Implementations should be lightweight and
 * non-blocking where possible for embedded use.
 */
class NotificationTransport {
   public:
    /**
     * @brief Virtual destructor to allow proper cleanup via base pointer.
     */
    virtual ~NotificationTransport() {}

    /**
     * @brief Send a notification using this transport.
     *
     * @param level Human-readable textual level (e.g. "INFO").
     * @param message Body of the notification.
     */
    virtual void send(const String& level, const String& message, unsigned long timestamp) = 0;
};

/**
 * @brief A simple transport that writes notifications to the Arduino Serial.
 *
 * This is the default transport used by Notification when no other transport
 * is provided. It writes a bracketed level followed by the message and a
 * newline, e.g. "[INFO] Hardware initialized".
 */
class SerialTransport : public NotificationTransport {
   public:
    /**
     * @brief Construct a new SerialTransport object and initialize Serial.
     * @param baud Baud rate for Serial communication (default 9600).
     */
    SerialTransport(unsigned long baud = 9600);

    /**
     * @brief Destroy the SerialTransport object.
     */
    ~SerialTransport() override;

    /**
     * @brief Initialize Serial communication.
     */
    void init();

    /**
     * @brief End Serial communication.
     */
    void end();

    /**
     * @brief Send a formatted message to Serial.
     *
     * @param level Textual representation of the log level (e.g. "INFO").
     * @param message Body of the notification to send.
     * @note This implementation is synchronous and will block while writing to Serial.
     */
    void send(const String& level, const String& message, unsigned long timestamp) override;

   private:
    unsigned long _baud;  // Baud rate for Serial communication
};

/**
 * @brief Notification helper with logging-like levels and pluggable transport.
 *
 * Usage examples:
 * - notifier.send(Notification::ERROR, "Something failed");
 * - notifier.info("Started");
 *
 * The constructor starts Serial at the default baud rate 9600. You may
 * replace the transport implementation at runtime using setTransport().
 */
class Notification {
   public:
    /**
     * @brief Notification severity levels.
     */
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    /**
     * @brief Construct a new Notification instance and initialize Serial.
     *
     * @param level Initial minimum level to emit. Messages with a level lower
     *              than this will be ignored. Default is INFO.
     * @param baud Serial baud rate to initialize (default 9600).
     * @param transport Optional transport implementation. If non-null, it will be used
     *                  but NOT owned by this Notification instance. If null, a
     *                  default SerialTransport will be allocated and owned.
     * @note If you pass a transport pointer and want Notification to take ownership
     *       pass takeOwnership=true when calling setTransport().
     */
    Notification(Level level = INFO, NotificationTransport* transport = nullptr);

    /**
     * @brief Destroy the Notification object.
     *
     * If the Notification instance owns the transport (setTransport was called with
     * takeOwnership=true or the default transport was created), the transport will
     * be deleted here.
     */
    ~Notification();

    /**
     * @brief Replace the current transport with a new one.
     *
     * @param transport Pointer to a NotificationTransport implementation (may be nullptr).
     * @param takeOwnership When true, Notification will delete the transport in its
     *                     destructor or when replaced.
     */
    void setTransport(NotificationTransport* transport, bool takeOwnership = false);

    /**
     * @brief Send a message at the given level.
     *
     * @param level Severity level from the Level enum.
     * @param message Message body to send.
     */
    void send(Level level, const String& message);

    /**
     * @brief Convenience helper: send a DEBUG-level notification.
     * @param message Message body to send.
     */
    void debug(const String& message);

    /**
     * @brief Convenience helper: send an INFO-level notification.
     * @param message Message body to send.
     */
    void info(const String& message);

    /**
     * @brief Convenience helper: send a WARNING-level notification.
     * @param message Message body to send.
     */
    void warning(const String& message);

    /**
     * @brief Convenience helper: send an ERROR-level notification.
     * @param message Message body to send.
     */
    void error(const String& message);

    /**
     * @brief Reconfigure the minimum level to emit.
     *
     * Messages with a level lower than the configured level will be ignored.
     * @param level New minimum level.
     */
    void setLevel(Level level);

    /**
     * @brief Get the currently configured minimum level.
     * @return Level Current minimum level.
     */
    Level getLevel() const;

   private:
    NotificationTransport* transport_;
    bool ownsTransport_;

    /**
     * @brief Current minimum level that will be emitted. Messages with
     * lower severity (numerically smaller) will be dropped.
     */
    Level currentLevel_;

    /**
     * @brief Convert the Level enum value to a human-readable String.
     *
     * @param level Enum value representing the log level.
     * @return String Textual name of the provided level ("DEBUG","INFO","WARNING","ERROR","UNKNOWN").
     */
    String levelToString(Level level);

    unsigned long initTime_;
};

#endif  // NOTIFICATION_H
