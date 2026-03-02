#pragma once

#include "Notification.h"
#include "SyncErrors.h"
#include "TimeMath.h"
#include "TimeSource.h"

/**
 * @file TimeCoordinator.h
 * @brief Central arbiter that selects the best time source with median-based coherence filtering.
 *
 * @details
 * Description:
 *   Queries all registered sources (DAB > NTP > GPS) every update cycle.
 *   For each source, it computes the median of the two other valid sources and
 *   rejects the source if it diverges by more than ±10 seconds.
 *   The "winning" source (highest priority among coherent ones) is used to update
 *   the internal reference time displayed on the LCD.
 *
 *   The coordinator also maintains a software clock (reference epoch + millis offset)
 *   so time continues to tick between sync cycles.
 *
 * @author GOLETTA David
 * @date 13/02/2026
 */
class TimeCoordinator {
   public:
    static constexpr uint8_t MAX_SOURCES = 3;
    static constexpr int32_t COHERENCE_THRESHOLD_S = 10;

    /**
     * @brief Construct the coordinator.
     * @details
     * Description:
     *   Initializes source slots, software clock state, and optional notification logging.
     *
     * @param notifier Optional logger for debug/warning messages.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    explicit TimeCoordinator(Notification* notifier = nullptr);

    /**
     * @brief Register time sources in priority order (index 0 = highest).
     * @details
     * Description:
     *   Stores the source list used by update(), preserving priority by array index.
     *
     * @param sources Array of pointers to TimeSource objects.
     * @param count   Number of sources (max MAX_SOURCES).
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void setSources(TimeSource* sources[], uint8_t count);

    /**
     * @brief Periodic update — call from loop().
     * @details
     * Description:
     *   Queries enabled sources, applies coherence filtering, picks the best valid source,
     *   updates the internal reference clock, and refreshes synchronization error state.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void update();

    /**
     * @brief Force an immediate resynchronization (called by SET when no errors pending).
     * @details
     * Description:
     *   Triggers a sync cycle without waiting for the normal periodic interval.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void forceSync();

    /**
     * @brief Get the current date-time to display (from the software clock).
     * @details
     * Description:
     *   Returns the coordinator-maintained reference date/time, advanced by elapsed millis.
     *
     * @param[out] out Filled with the current reference date-time.
     * @return True if a valid reference exists.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    bool getCurrentDateTime(DateTimeFields& out) const;

    /**
     * @brief Override the reference time (used by manual configuration mode).
     * @details
     * Description:
     *   Replaces the internal software-clock reference with a manually supplied date-time.
     *
     * @param dt New date-time to set as reference.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void setManualDateTime(const DateTimeFields& dt);

    /**
     * @brief Access the error state (for UI display and acknowledgement).
     * @details
     * Description:
     *   Provides mutable access to the synchronization error container.
     *
     * @return Reference to the internal SyncErrorState object.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    SyncErrorState& errors();

    /**
     * @brief Access the error state (read-only).
     * @details
     * Description:
     *   Provides const access to the synchronization error container.
     *
     * @return Const reference to the internal SyncErrorState object.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    const SyncErrorState& errors() const;

    /**
     * @brief Returns true if at least one source provided valid time this cycle.
     * @details
     * Description:
     *   Indicates whether the latest update cycle produced at least one valid source reading.
     *
     * @return True if one or more sources were valid in the current cycle; false otherwise.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    bool hasValidSource() const;

    /**
     * @brief Index of the source currently providing time (0=DAB,1=NTP,2=GPS), or -1.
     * @details
     * Description:
     *   Returns the selected source index chosen during the latest update cycle.
     *
     * @return Active source index in [0..2], or -1 when no source is active.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    int8_t activeSourceIndex() const;

   private:
    void checkCoherence();
    void selectBestSource();
    void updateSoftwareClock();

    TimeSource* sources_[MAX_SOURCES] = {};
    uint8_t sourceCount_ = 0;
    Notification* notifier_ = nullptr;

    // Per-source query results for the current cycle
    bool sourceValid_[MAX_SOURCES] = {};
    DateTimeFields sourceDateTime_[MAX_SOURCES] = {};
    uint32_t sourceEpoch_[MAX_SOURCES] = {};
    bool sourceCoherent_[MAX_SOURCES] = {true, true, true};

    // Winning source for this cycle
    int8_t activeSource_ = -1;
    bool hasValid_ = false;

    // Software clock reference
    bool hasReference_ = false;
    uint32_t referenceEpoch_ = 0;
    uint32_t referenceMillis_ = 0;

    // Sync timing
    uint32_t lastSyncMs_ = 0;
    static constexpr uint32_t SYNC_INTERVAL_MS = 1000;

    SyncErrorState errorState_{};
};
