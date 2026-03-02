#pragma once

#include <Arduino.h>

#include "TimeSource.hpp"

/**
 * @file TimeMath.h
 * @brief Date/time arithmetic utilities (epoch conversion, delta computation).
 *
 * @details
 * Description:
 *   Provides functions to convert between DateTimeFields and a Unix-style epoch
 *   representation, and to compute signed deltas in seconds between two date-times.
 *   These are used by TimeCoordinator to detect source incoherence (±10 s).
 *
 * @author GOLETTA David
 * @date 13/02/2026
 */
namespace TimeMath {

/**
 * @brief Check whether a year is a leap year.
 * @details
 * Description:
 *   Applies Gregorian calendar leap-year rules to the provided year value.
 *
 * @param year Full year (e.g. 2026).
 * @return True if the year is a leap year.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 */
inline bool isLeapYear(uint16_t year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return (year % 4 == 0);
}

/**
 * @brief Number of days in a given month.
 * @details
 * Description:
 *   Returns the month length and handles leap-year February for the given year.
 *
 * @param year Full year (for leap-year February check).
 * @param month Month number 1-12.
 * @return Number of days (28-31).
 *
 * @author GOLETTA David
 * @date 02/03/2026
 */
inline uint8_t daysInMonth(uint16_t year, uint8_t month) {
    static const uint8_t kDaysPerMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && isLeapYear(year)) return 29;
    return kDaysPerMonth[month - 1];
}

/**
 * @brief Convert DateTimeFields to a Unix-style epoch (seconds since 1970-01-01 00:00:00 UTC).
 * @details
 * Description:
 *   Computes elapsed whole days and time-of-day seconds to produce a Unix-style epoch value.
 *
 * @param dt DateTimeFields to convert.
 * @return Epoch in seconds. Returns 0 if the date/time is clearly invalid.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 */
inline uint32_t toEpoch(const DateTimeFields& dt) {
    if (dt.date.year < 1970 || dt.date.month == 0 || dt.date.month > 12 ||
        dt.date.day == 0 || dt.date.day > 31) {
        return 0;
    }

    uint32_t days = 0;

    // Full years since 1970
    for (uint16_t y = 1970; y < dt.date.year; ++y) {
        days += isLeapYear(y) ? 366 : 365;
    }

    // Full months in the current year
    for (uint8_t m = 1; m < dt.date.month; ++m) {
        days += daysInMonth(dt.date.year, m);
    }

    // Days in the current month (day is 1-based)
    days += dt.date.day - 1;

    uint32_t secs = days * 86400UL;
    secs += static_cast<uint32_t>(dt.time.hour) * 3600UL;
    secs += static_cast<uint32_t>(dt.time.minute) * 60UL;
    secs += dt.time.second;

    return secs;
}

/**
 * @brief Convert a Unix epoch to DateTimeFields.
 * @details
 * Description:
 *   Decomposes epoch seconds into calendar date and time components.
 *
 * @param epochSeconds Seconds since 1970-01-01 00:00:00 UTC.
 * @param[out] out Structure to fill.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 */
inline void fromEpoch(uint32_t epochSeconds, DateTimeFields& out) {
    uint32_t secondsOfDay = epochSeconds % 86400UL;
    uint32_t days = epochSeconds / 86400UL;

    out.time.hour = static_cast<uint8_t>(secondsOfDay / 3600UL);
    secondsOfDay %= 3600UL;
    out.time.minute = static_cast<uint8_t>(secondsOfDay / 60UL);
    out.time.second = static_cast<uint8_t>(secondsOfDay % 60UL);

    uint16_t year = 1970;
    while (true) {
        uint16_t daysThisYear = isLeapYear(year) ? 366 : 365;
        if (days < daysThisYear) break;
        days -= daysThisYear;
        ++year;
    }

    uint8_t month = 1;
    while (true) {
        uint8_t dim = daysInMonth(year, month);
        if (days < dim) break;
        days -= dim;
        ++month;
    }

    out.date.year = year;
    out.date.month = month;
    out.date.day = static_cast<uint8_t>(days + 1);
}

/**
 * @brief Compute the signed delta (a - b) in seconds between two DateTimeFields.
 * @details
 * Description:
 *   Converts both date-time values to epoch seconds, then returns the signed difference.
 *
 * @param a First date-time.
 * @param b Second date-time.
 * @return Signed difference in seconds (positive if a is later than b).
 *
 * @author GOLETTA David
 * @date 02/03/2026
 */
inline int32_t deltaSeconds(const DateTimeFields& a, const DateTimeFields& b) {
    uint32_t epochA = toEpoch(a);
    uint32_t epochB = toEpoch(b);
    return static_cast<int32_t>(epochA) - static_cast<int32_t>(epochB);
}

}  // namespace TimeMath
