#pragma once

#include <DABShield.h>

#include "Notification.h"
#include "TimeSource.h"

/**
 * @brief Time source implementation backed by a DAB (Digital Audio Broadcasting) receiver.
 * @details
 * Description:
 *   Provides date/time values retrieved from a DABShield instance.
 *   Applies a fixed timezone offset to the hour field (hour wrapping only).
 *
 * @par Inputs
 *   Requires references to an initialized DAB object, a DABTime storage struct, and a shared hasService flag.
 *
 * @par Outputs
 *   getDateTime/getTime/getDate return true when DAB time is available and the source is enabled.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
class DABTimeSource : public TimeSource {
   public:
    /**
     * @brief Construct a DABTimeSource.
     * @details
     * Description:
     *   Stores references and configuration needed to read time/date from the provided DAB objects.
     *
     * @param dab Reference to the DABShield driver instance.
     * @param dabtime Reference to a DABTime structure that will be filled by the library.
     * @param hasService Reference to a flag tracking whether a DAB service is available.
     * @param timezoneOffsetHours Hour offset applied to DAB-provided hour.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    DABTimeSource(DAB& dab, DABTime& dabtime, bool& hasService, int8_t timezoneOffsetHours = 0, Notification* notifier = nullptr);

    /**
     * @brief Initialize the DAB shield and attempt to tune a service.
     * @details
     * Description:
     *   Performs the SPI-related configuration used by the sketch, initializes the DAB shield, and scans
     *   for the first available DAB service to tune.
     *
     * @param spiSelectPin SPI chip select pin for the DAB shield.
     * @param speakerOutput Speaker output mode.
     * @param band Band selection (0 = DAB band, 1 = FM band).
     * @return True if initialization succeeded and at least one service is available; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool init(uint8_t spiSelectPin, uint8_t speakerOutput, uint8_t band = 0);

    /**
     * @brief Check if the current DAB date and time values are sane.
     * @details
     * Description:
     *   Validates the DAB-provided date and time values to ensure they fall within acceptable ranges.
     *
     * @return True if the date and time values are sane; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool isSaneDateTime() const;

    /**
     * @brief Get the current date and time from DAB.
     * @details
     * Description:
     *   Queries DAB status/time and fills out when a valid time is available and the source is enabled.
     *
     * @param out Reference to a DateTimeFields structure to be filled.
     * @return True if out contains valid DAB-derived values; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool getDateTime(DateTimeFields& out) override;

   private:
    /**
     * @brief Tune the first available DAB service.
     * @details
     * Description:
     *   Iterates through known DAB frequencies and attempts to tune a valid service.
     *
     * @return True if a valid service is found and selected; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool tuneFirstAvailableService();

    DAB& dab_;
    DABTime& dabtime_;
    bool& hasService_;
    int8_t timezoneOffsetHours_;
    Notification* notifier_;
    uint8_t tunedServiceIndex_ = 0;
    DateTimeFields cachedDateTime_{};
    bool hasCachedDateTime_ = false;
};
