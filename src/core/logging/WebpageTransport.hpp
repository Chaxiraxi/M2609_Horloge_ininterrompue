#pragma once

#include <Arduino.h>

#include "Notification.hpp"

/**
 * @brief Notification transport that keeps recent logs in memory for the web UI.
 * @details
 * Description:
 *   Stores a fixed-size circular buffer of recent messages and exposes read access
 *   for HTTP serialization.
 *
 * @author GOLETTA David
 * @date 04/03/2026
 */
class WebpageTransport : public NotificationTransport {
   public:
    /**
     * @brief Number of log entries retained in memory.
     * @details
     * Description:
     *   The transport keeps at most 40 entries; oldest entries are overwritten.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    static constexpr uint8_t MAX_LOG_COUNT = 40;

    /**
     * @brief Maximum retained message text length (excluding null terminator).
     * @details
     * Description:
     *   Messages longer than this limit are truncated before storage to keep RAM usage bounded.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    static constexpr uint8_t MAX_LOG_MESSAGE_CHARS = 63;

    /**
     * @brief Lightweight log entry representation used by API serialization.
     * @details
     * Description:
     *   Contains timestamp (ms since logger init), normalized level, and message text.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    struct LogEntry {
        uint32_t timestampMs = 0;
        char level[8] = {};
        char message[MAX_LOG_MESSAGE_CHARS + 1] = {};
    };

    /**
     * @brief Construct an empty WebpageTransport.
     * @details
     * Description:
     *   Initializes internal ring buffer cursors and clears retained entries.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    WebpageTransport();

    /**
     * @brief Destroy the WebpageTransport object.
     * @details
     * Description:
     *   No dynamic memory is owned by this transport.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    ~WebpageTransport() override;

    /**
     * @brief Store one incoming notification into the ring buffer.
     * @details
     * Description:
     *   Normalizes the textual level and appends the message, overwriting the oldest
     *   record when capacity is reached.
     *
     * @param level Textual representation of the log level.
     * @param message Body of the log message.
     * @param timestamp Milliseconds elapsed since Notification initialization.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    void send(const String& level, const String& message, unsigned long timestamp) override;

    /**
     * @brief Get current number of stored entries.
     * @details
     * Description:
     *   Returns how many valid entries are currently retained in the circular buffer.
     *
     * @return Number of retained log entries.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    uint8_t count() const;

    /**
     * @brief Read one entry by recency order.
     * @details
     * Description:
     *   Index 0 is the newest entry, index 1 the previous one, and so on.
     *
     * @param indexFromNewest Position in recency order.
     * @param outEntry Output log entry when index is valid.
     * @return True when the entry exists and was copied.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    bool getNewest(uint8_t indexFromNewest, LogEntry& outEntry) const;

    /**
     * @brief Clear all retained entries.
     * @details
     * Description:
     *   Resets count/head cursors and empties in-memory logs.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    void clear();

   private:
    /**
     * @internal
     * @brief Normalize and copy a level string into a fixed buffer.
     * @details
     * Trims spaces and maps unknown values to "INFO".
     *
     * @param inputLevel Source level text.
     * @param outLevel Destination fixed-size C string.
     * @param outSize Destination buffer size in bytes.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     * @endinternal
     */
    void normalizeLevel(const String& inputLevel, char* outLevel, size_t outSize) const;

    /**
     * @internal
     * @brief Copy message text into fixed buffer.
     * @details
     * Copies up to `outSize - 1` chars and replaces line breaks by spaces.
     *
     * @param inputMessage Source message text.
     * @param outMessage Destination fixed-size C string.
     * @param outSize Destination buffer size in bytes.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     * @endinternal
     */
    void copyMessage(const String& inputMessage, char* outMessage, size_t outSize) const;

    LogEntry entries_[MAX_LOG_COUNT] = {};
    uint8_t nextWriteIndex_ = 0;
    uint8_t count_ = 0;
};
