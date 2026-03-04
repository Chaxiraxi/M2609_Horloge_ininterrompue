#pragma once

#include <Adafruit_GPS.h>

#include "../core/TimeSource.hpp"

/**
 * @brief Time source implementation backed by an Adafruit GPS receiver.
 * @details
 * Description:
 *   Provides date/time values obtained from the Adafruit_GPS library (NMEA parsing done elsewhere).
 *   Applies a timezone offset in minutes and adjusts both date and time.
 *
 * @par Inputs
 *   Requires a reference to an already-constructed Adafruit_GPS instance.
 *
 * @par Outputs
 *   getDateTime/getTime/getDate return true when the GPS has a sane timestamp and the source is enabled.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
class GPSTimeSource : public TimeSource {
   public:
    /**
     * @brief Construct a GPSTimeSource.
     * @details
     * Description:
     *   Stores references and configuration needed to read time/date from the provided GPS object.
     *
     * @param gps Reference to an Adafruit_GPS instance.
     * @param timezoneOffsetMinutes Offset applied to GPS UTC time in minutes (e.g., +60 for CET).
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    GPSTimeSource(Adafruit_GPS& gps, int16_t timezoneOffsetMinutes = 0);

    /**
     * @brief Initialize the GPS module configuration.
     * @details
     * Description:
     *   Configures the GPS module via the Adafruit_GPS API (baud rate, sentence output, update rate,
     *   antenna status, and firmware version request).
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    void init();

    /**
     * @brief Get the current date and time from GPS.
     * @details
     * Description:
     *   Reads timestamp fields from the underlying Adafruit_GPS object and fills out if the fields
     *   are considered sane and the source is enabled.
     *
     * @param out Reference to a DateTimeFields structure to be filled.
     * @return True if out contains valid GPS-derived values; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool getDateTime(DateTimeFields& out) override;

    /**
     * @brief Update timezone offset for GPS-derived time.
     * @details
     * Description:
     *   Sets the signed offset in minutes that is applied to GPS UTC timestamps.
     *
     * @param offsetMinutes Signed offset in minutes from UTC.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    void setTimezoneOffsetMinutes(int16_t offsetMinutes) override;

    /**
     * @brief Read current timezone offset for this source.
     * @details
     * Description:
     *   Returns the signed offset in minutes currently used for conversion.
     *
     * @return Signed offset in minutes from UTC.
     *
     * @author GOLETTA David
     * @date 04/03/2026
     */
    int16_t getTimezoneOffsetMinutes() const override;

   private:
    /**
     * @brief Validate the GPS date/time fields.
     * @details
     * Description:
     *   Checks that the GPS timestamp fields are non-zero and within reasonable ranges.
     *
     * @return True if the internal GPS fields look valid; false otherwise.
     *
     * @author GOLETTA David
     * @date 11/02/2026
     */
    bool isSaneDateTime() const;

    Adafruit_GPS& gps_;
    int16_t timezoneOffsetMinutes_;
};
