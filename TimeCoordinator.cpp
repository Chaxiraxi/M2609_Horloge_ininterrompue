#include "TimeCoordinator.h"

#include <algorithm>

TimeCoordinator::TimeCoordinator(Notification* notifier)
    : notifier_(notifier) {}

void TimeCoordinator::setSources(TimeSource* sources[], uint8_t count) {
    sourceCount_ = min(count, MAX_SOURCES);
    for (uint8_t i = 0; i < sourceCount_; ++i) {
        sources_[i] = sources[i];
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void TimeCoordinator::update() {
    uint32_t now = millis();
    if (now - lastSyncMs_ < SYNC_INTERVAL_MS && hasReference_) {
        // Between syncs, just let the software clock run.
        return;
    }
    lastSyncMs_ = now;

    // 1. Query every enabled source
    hasValid_ = false;
    for (uint8_t i = 0; i < sourceCount_; ++i) {
        sourceValid_[i] = false;
        sourceCoherent_[i] = true;  // innocent until proven guilty

        if (!sources_[i] || !sources_[i]->isEnabled()) continue;

        DateTimeFields dt{};
        if (sources_[i]->getDateTime(dt)) {
            uint32_t epoch = TimeMath::toEpoch(dt);
            if (epoch != 0) {
                sourceValid_[i] = true;
                sourceDateTime_[i] = dt;
                sourceEpoch_[i] = epoch;
                hasValid_ = true;
            }
        }
    }

    // 2. Coherence check (median of two others)
    checkCoherence();

    // 3. Select best coherent source according to priority (index order)
    selectBestSource();

    // 4. Update software clock
    updateSoftwareClock();

    // 5. Update error state — auto-ack if applicable
    errorState_.autoAcknowledge(hasValid_);
}

void TimeCoordinator::forceSync() {
    // Reset the sync timer so the next update() actually queries sources.
    lastSyncMs_ = 0;
}

bool TimeCoordinator::getCurrentDateTime(DateTimeFields& out) const {
    if (!hasReference_) return false;

    // Compute current epoch by adding elapsed millis since last reference update.
    uint32_t elapsed = millis() - referenceMillis_;
    uint32_t currentEpoch = referenceEpoch_ + elapsed / 1000UL;
    TimeMath::fromEpoch(currentEpoch, out);
    return true;
}

void TimeCoordinator::setManualDateTime(const DateTimeFields& dt) {
    referenceEpoch_ = TimeMath::toEpoch(dt);
    referenceMillis_ = millis();
    hasReference_ = true;
}

SyncErrorState& TimeCoordinator::errors() {
    return errorState_;
}

const SyncErrorState& TimeCoordinator::errors() const {
    return errorState_;
}

bool TimeCoordinator::hasValidSource() const {
    return hasValid_;
}

int8_t TimeCoordinator::activeSourceIndex() const {
    return activeSource_;
}

// ---------------------------------------------------------------------------
// Coherence — median of the two other sources
// ---------------------------------------------------------------------------

// TODO: Validate this function since it always marks 2 sources as incoherent
/**
 * @internal
 * @brief Evaluate source coherence against peers.
 * @details
 * For each valid source, computes a median reference from other valid sources and
 * marks the source incoherent when its deviation exceeds `COHERENCE_THRESHOLD_S`.
 * Updates per-source error flags accordingly.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void TimeCoordinator::checkCoherence() {
    // We need at least 2 valid sources to perform any coherence check.
    uint8_t validCount = 0;
    for (uint8_t i = 0; i < sourceCount_; ++i) {
        if (sourceValid_[i]) ++validCount;
    }

    if (validCount < 2) {
        // Can't check coherence with fewer than 2 sources — accept everything.
        // Clear any previous incoherence errors that may no longer apply.
        for (uint8_t i = 0; i < sourceCount_; ++i) {
            sourceCoherent_[i] = true;
        }
        return;
    }

    // For each valid source, compute the median of the other valid sources
    // and check if it diverges by more than COHERENCE_THRESHOLD_S.
    for (uint8_t i = 0; i < sourceCount_; ++i) {
        if (!sourceValid_[i]) continue;

        // Collect epochs of the other valid sources
        uint32_t others[MAX_SOURCES];
        uint8_t otherCount = 0;
        for (uint8_t j = 0; j < sourceCount_; ++j) {
            if (j == i || !sourceValid_[j]) continue;
            others[otherCount++] = sourceEpoch_[j];
        }

        if (otherCount == 0) {
            // Only this source is valid — accept implicitly
            sourceCoherent_[i] = true;
            continue;
        }

        // Compute median of others (with 1 value it's itself, with 2 it's the average)
        uint32_t median;
        if (otherCount == 1) {
            median = others[0];
        } else {
            // Sort and take middle (for 2 elements, average; for 3+, true median)
            std::sort(others, others + otherCount);
            if (otherCount % 2 == 0) {
                median = (others[otherCount / 2 - 1] + others[otherCount / 2]) / 2;
            } else {
                median = others[otherCount / 2];
            }
        }

        int32_t delta = static_cast<int32_t>(sourceEpoch_[i]) - static_cast<int32_t>(median);
        if (abs(delta) > COHERENCE_THRESHOLD_S) {
            sourceCoherent_[i] = false;
            if (notifier_) {
                notifier_->warning("Source " + String(i) + " incoherent (delta=" + String(delta) + "s)");
            }
            // Raise per-source incoherence error
            SyncErrorCode errCode;
            switch (i) {
                case SyncErrorState::IDX_DAB:
                    errCode = SyncErrorCode::DAB_INCOHERENT;
                    break;
                case SyncErrorState::IDX_NTP:
                    errCode = SyncErrorCode::NTP_INCOHERENT;
                    break;
                case SyncErrorState::IDX_GPS:
                    errCode = SyncErrorCode::GPS_INCOHERENT;
                    break;
                default:
                    errCode = SyncErrorCode::NONE;
                    break;
            }
            errorState_.sourceErrors[i].raise(errCode);
        } else {
            sourceCoherent_[i] = true;
            // Clear incoherence error if it was previously set
            errorState_.sourceErrors[i].clear();
        }
    }
}

// ---------------------------------------------------------------------------
// Select best source (highest priority = lowest index) among valid + coherent
// ---------------------------------------------------------------------------

/**
 * @internal
 * @brief Select the active source from coherent candidates.
 * @details
 * Chooses the first source by priority order that is both valid and coherent, and
 * updates the global no-source error when none is suitable.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void TimeCoordinator::selectBestSource() {
    activeSource_ = -1;

    for (uint8_t i = 0; i < sourceCount_; ++i) {
        if (sourceValid_[i] && sourceCoherent_[i]) {
            activeSource_ = static_cast<int8_t>(i);
            break;
        }
    }

    if (activeSource_ < 0) {
        // No coherent source available — check if any source was valid at all
        if (!hasValid_) {
            errorState_.globalError.raise(SyncErrorCode::NO_SOURCE_AVAILABLE);
        }
    } else {
        errorState_.globalError.clear();
    }
}

// ---------------------------------------------------------------------------
// Software clock — keeps time ticking between sync cycles
// ---------------------------------------------------------------------------

/**
 * @internal
 * @brief Refresh software-clock reference from active source.
 * @details
 * When an active source exists, stores its epoch as the new reference point and
 * records the current `millis()` timestamp used for elapsed-time extrapolation.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void TimeCoordinator::updateSoftwareClock() {
    if (activeSource_ >= 0) {
        referenceEpoch_ = sourceEpoch_[activeSource_];
        referenceMillis_ = millis();
        hasReference_ = true;
    }
    // If no active source, keep the old reference running (software clock
    // continues from the last known time).
}
