#pragma once

#include <Arduino.h>

/**
 * @file SyncErrors.h
 * @brief Error model for time-synchronization status reporting.
 *
 * @details
 * Description:
 *   Defines error codes and a lightweight SyncError struct used by the
 *   TimeCoordinator and UiController to track, display, and acknowledge
 *   synchronization issues.
 *
 * @author GOLETTA David
 * @date 13/02/2026
 */

/**
 * @brief Possible synchronization error codes.
 */
enum class SyncErrorCode : uint8_t {
    NONE = 0,
    NO_SOURCE_AVAILABLE,    ///< No enabled source provided a valid time
    DAB_NO_SERVICE,         ///< DAB shield found no service
    DAB_SERVICE_LOST,       ///< DAB service was lost
    DAB_INCOHERENT,         ///< DAB time diverges >10 s from median
    NTP_WIFI_DISCONNECTED,  ///< WiFi not connected, NTP unavailable
    NTP_NO_TIME,            ///< NTP update returned epoch 0
    NTP_INCOHERENT,         ///< NTP time diverges >10 s from median
    GPS_INVALID_TIME,       ///< GPS fields not sane
    GPS_INCOHERENT,         ///< GPS time diverges >10 s from median
};

/**
 * @brief Single synchronization error entry.
 */
struct SyncError {
    SyncErrorCode code = SyncErrorCode::NONE;
    uint32_t createdAtMs = 0;   ///< millis() when the error was raised
    bool acknowledged = false;  ///< True once the user pressed SET to ack

    /**
     * @brief Check if this error is active (non-NONE and not yet acknowledged).
     * @details
     * Description:
     *   Returns true when an error code is set and the entry has not been acknowledged.
     *
     * @return True if this error is active; false otherwise.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    bool isActive() const {
        return code != SyncErrorCode::NONE && !acknowledged;
    }

    /**
     * @brief Acknowledge (dismiss) this error.
     * @details
     * Description:
     *   Marks the current error as acknowledged so it is no longer considered active.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void acknowledge() {
        acknowledged = true;
    }

    /**
     * @brief Clear the error entirely.
     * @details
     * Description:
     *   Resets the code, timestamp, and acknowledgement flag to the default no-error state.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void clear() {
        code = SyncErrorCode::NONE;
        createdAtMs = 0;
        acknowledged = false;
    }

    /**
     * @brief Raise (or re-raise) an error.
     * @details
     * Description:
     *   Sets the provided error code, updates the creation timestamp, and marks it unacknowledged.
     *   If the same code is already present, no state change is applied.
     *
     * @param c Error code.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void raise(SyncErrorCode c) {
        if (code == c) return;  // already raised with same code
        code = c;
        createdAtMs = millis();
        acknowledged = false;
    }

    /**
     * @brief Get a short human-readable label for the error (fits 16 chars for LCD scroll).
     * @details
     * Description:
     *   Converts the current error code to a compact display label suitable for the LCD.
     *
     * @return Pointer to a static C-string label corresponding to the current error code.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    const char* label() const {
        switch (code) {
            case SyncErrorCode::NONE:
                return "";
            case SyncErrorCode::NO_SOURCE_AVAILABLE:
                return "No source";
            case SyncErrorCode::DAB_NO_SERVICE:
                return "DAB no svc";
            case SyncErrorCode::DAB_SERVICE_LOST:
                return "DAB svc lost";
            case SyncErrorCode::DAB_INCOHERENT:
                return "DAB incoh.";
            case SyncErrorCode::NTP_WIFI_DISCONNECTED:
                return "WiFi lost";
            case SyncErrorCode::NTP_NO_TIME:
                return "NTP no time";
            case SyncErrorCode::NTP_INCOHERENT:
                return "NTP incoh.";
            case SyncErrorCode::GPS_INVALID_TIME:
                return "GPS invalid";
            case SyncErrorCode::GPS_INCOHERENT:
                return "GPS incoh.";
            default:
                return "Err?";
        }
    }
};

/**
 * @brief Container for all current sync errors (one per source + one global).
 */
struct SyncErrorState {
    static constexpr uint8_t SOURCE_COUNT = 3;  ///< DAB, NTP, GPS
    static constexpr uint8_t IDX_DAB = 0;
    static constexpr uint8_t IDX_NTP = 1;
    static constexpr uint8_t IDX_GPS = 2;

    SyncError sourceErrors[SOURCE_COUNT];  ///< Per-source error
    SyncError globalError;                 ///< e.g. NO_SOURCE_AVAILABLE

    static constexpr uint32_t AUTO_ACK_TIMEOUT_MS = 60000;  ///< 60 s

    /**
     * @brief Check if any error is active (not acknowledged).
     * @details
     * Description:
     *   Evaluates global and per-source entries and reports whether at least one active error exists.
     *
     * @return True if one or more errors are active; false otherwise.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    bool hasActiveError() const {
        if (globalError.isActive()) return true;
        for (uint8_t i = 0; i < SOURCE_COUNT; ++i) {
            if (sourceErrors[i].isActive()) return true;
        }
        return false;
    }

    /**
     * @brief Check if ALL errors have been acknowledged (none active).
     * @details
     * Description:
     *   Convenience helper equivalent to the negation of hasActiveError().
     *
     * @return True when no active unacknowledged errors remain; false otherwise.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    bool allAcknowledged() const {
        return !hasActiveError();
    }

    /**
     * @brief Acknowledge all currently active errors.
     * @details
     * Description:
     *   Marks the global error and all source errors as acknowledged.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void acknowledgeAll() {
        globalError.acknowledge();
        for (uint8_t i = 0; i < SOURCE_COUNT; ++i) {
            sourceErrors[i].acknowledge();
        }
    }

    /**
     * @brief Auto-acknowledge errors older than AUTO_ACK_TIMEOUT_MS when
     *        at least one valid source exists.
     * @details
     * Description:
     *   Automatically acknowledges active errors once they exceed the configured timeout,
     *   but only if at least one source currently provides valid time.
     *
     * @param hasValidSource True if at least one source currently provides time.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void autoAcknowledge(bool hasValidSource) {
        if (!hasValidSource) return;
        uint32_t now = millis();
        if (globalError.isActive() && (now - globalError.createdAtMs >= AUTO_ACK_TIMEOUT_MS)) {
            globalError.acknowledge();
        }
        for (uint8_t i = 0; i < SOURCE_COUNT; ++i) {
            if (sourceErrors[i].isActive() && (now - sourceErrors[i].createdAtMs >= AUTO_ACK_TIMEOUT_MS)) {
                sourceErrors[i].acknowledge();
            }
        }
    }

    /**
     * @brief Get the first active error to display. Prioritises global, then DAB, NTP, GPS.
     * @details
     * Description:
     *   Returns the highest-priority active error entry for UI rendering.
     *
     * @return Pointer to the first active SyncError, or nullptr if none.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    const SyncError* firstActive() const {
        if (globalError.isActive()) return &globalError;
        for (uint8_t i = 0; i < SOURCE_COUNT; ++i) {
            if (sourceErrors[i].isActive()) return &sourceErrors[i];
        }
        return nullptr;
    }
};
