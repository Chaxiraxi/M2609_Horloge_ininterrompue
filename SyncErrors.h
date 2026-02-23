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
     */
    bool isActive() const {
        return code != SyncErrorCode::NONE && !acknowledged;
    }

    /**
     * @brief Acknowledge (dismiss) this error.
     */
    void acknowledge() {
        acknowledged = true;
    }

    /**
     * @brief Clear the error entirely.
     */
    void clear() {
        code = SyncErrorCode::NONE;
        createdAtMs = 0;
        acknowledged = false;
    }

    /**
     * @brief Raise (or re-raise) an error.
     * @param c Error code.
     */
    void raise(SyncErrorCode c) {
        if (code == c) return;  // already raised with same code
        code = c;
        createdAtMs = millis();
        acknowledged = false;
    }

    /**
     * @brief Get a short human-readable label for the error (fits 16 chars for LCD scroll).
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
     */
    bool allAcknowledged() const {
        return !hasActiveError();
    }

    /**
     * @brief Acknowledge all currently active errors.
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
     * @param hasValidSource True if at least one source currently provides time.
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
     * @return Pointer to the first active SyncError, or nullptr if none.
     */
    const SyncError* firstActive() const {
        if (globalError.isActive()) return &globalError;
        for (uint8_t i = 0; i < SOURCE_COUNT; ++i) {
            if (sourceErrors[i].isActive()) return &sourceErrors[i];
        }
        return nullptr;
    }
};
