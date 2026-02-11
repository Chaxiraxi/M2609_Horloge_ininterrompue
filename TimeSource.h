#pragma once

#include <Arduino.h>

/**
 * @brief Simple container for a time-of-day.
 * @details
 * Description:
 *   Holds hour/minute/second fields used throughout the project.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
struct TimeFields {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

/**
 * @brief Simple container for a calendar date.
 * @details
 * Description:
 *   Holds year/month/day fields used throughout the project.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
struct DateFields {
    uint16_t year;
    uint8_t month;
    uint8_t day;
};

/**
 * @brief Combines DateFields and TimeFields.
 * @details
 * Description:
 *   Represents a full date-time (date + time) as separate sub-structures.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
struct DateTimeFields {
    DateFields date;
    TimeFields time;
};

/**
 * @brief Wrap an hour value to the [0..23] range.
 * @details
 * Description:
 *   Normalizes an hour that may be outside the standard range (e.g., after applying a timezone offset).
 *   This function only wraps the hour value and does not adjust the date.
 *
 * @param hours Hour value that may be < 0 or >= 24.
 * @return Wrapped hour in the [0..23] range.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
inline uint8_t wrapHours(int32_t hours) {
    while (hours < 0) {
        hours += 24;
    }
    while (hours >= 24) {
        hours -= 24;
    }
    return static_cast<uint8_t>(hours);
}

/**
 * @brief Abstract base class for a time/date provider.
 * @details
 * Description:
 *   Defines a common interface for time sources (e.g., GPS, DAB). A source can be enabled/disabled,
 *   and can provide time, date, or date-time values through a consistent API.
 *
 * @par Inputs
 *   Derived classes provide the hardware/library dependencies (e.g., references to GPS/DAB objects).
 *
 * @par Outputs
 *   Implementations return true when valid data is available, false otherwise.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
class TimeSource {
   public:
    /**
     * @brief Virtual destructor.
     * @details
     * Description:
     *   Ensures derived classes are destroyed correctly via a base pointer.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    virtual ~TimeSource() = default;

    /**
     * @brief Enable or disable this time source.
     * @details
     * Description:
     *   When disabled, the source should be treated as unavailable and its getters should return false.
     *
     * @param enabled True to enable the source, false to disable it.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    void setEnabled(bool enabled) {
        enabled_ = enabled;
    }

    /**
     * @brief Query whether the source is enabled.
     * @details
     * Description:
     *   Returns the current enabled/disabled state of this time source.
     *
     * @return True if enabled, false if disabled.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool isEnabled() const {
        return enabled_;
    }

    /**
     * @brief Get the current date and time from the source.
     * @details
     * Description:
     *   Primary API implemented by derived classes. Should populate out with valid fields when
     *   time is available.
     *
     * @param out Reference to a DateTimeFields structure to be filled.
     * @return True if out contains valid data; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    virtual bool getDateTime(DateTimeFields& out) = 0;

    /**
     * @brief Get only the time-of-day from the source.
     * @details
     * Description:
     *   Convenience method. Default implementation calls getDateTime() and extracts the time part.
     *
     * @param out Reference to a TimeFields structure to be filled.
     * @return True if out contains valid data; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    virtual bool getTime(TimeFields& out) {
        DateTimeFields dateTime;
        if (!getDateTime(dateTime)) {
            return false;
        }
        out = dateTime.time;
        return true;
    }

    /**
     * @brief Get only the calendar date from the source.
     * @details
     * Description:
     *   Convenience method. Default implementation calls getDateTime() and extracts the date part.
     *
     * @param out Reference to a DateFields structure to be filled.
     * @return True if out contains valid data; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    virtual bool getDate(DateFields& out) {
        DateTimeFields dateTime;
        if (!getDateTime(dateTime)) {
            return false;
        }
        out = dateTime.date;
        return true;
    }

   protected:
    /**
     * @brief Enabled/disabled state of this source.
     * @details
     * Description:
     *   Derived classes should respect this flag in their getters.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool enabled_ = true;
};
