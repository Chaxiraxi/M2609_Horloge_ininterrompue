#pragma once

#include <Adafruit_GPS.h>
#include <Arduino.h>
#include <DABShield.h>
#include <LiquidCrystalIO.h>

/**
 * @file utils.h
 * @brief Declarations for shared display and formatting utilities.
 *
 * @details
 * Description:
 *   Provides helper functions for printing time/date information to Serial
 *   and for formatting output on the LCD used by the project.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */

/**
 * @brief Print a formatted DAB time line to Serial.
 * @details
 * Description:
 *   Prints the current DAB date/time fields in a human-readable format.
 *
 * @param dabtime Reference to a DABTime structure with valid values.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void printDabTime(const DABTime& dabtime);

/**
 * @brief Print a formatted GPS time line to Serial.
 * @details
 * Description:
 *   Prints the current GPS date/time fields in a human-readable format.
 *
 * @param gps Reference to an Adafruit_GPS instance with valid values.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void printGpsTime(const Adafruit_GPS& gps);

/**
 * @brief Print a two-digit number to the LCD.
 * @details
 * Description:
 *   Adds a leading zero for values below 10 before printing the value.
 *
 * @param lcd Reference to the LCD instance used for output.
 * @param value Value to print (0..99 expected).
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void printTwoDigits(LiquidCrystal& lcd, uint8_t value);

/**
 * @brief Convert a date to the day-of-week string.
 * @details
 * Description:
 *   Uses Zeller's Congruence to compute the day of the week for a given date.
 *
 * @param year Full year (e.g., 2026).
 * @param month Month (1..12).
 * @param day Day of month (1..31).
 * @param shortForm True to return abbreviated names (e.g., Mon), false for full names.
 * @return Day-of-week string, or an empty string if the computation fails.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
String dateToDayOfTheWeek(uint16_t year, uint8_t month, uint8_t day, bool shortForm = false);

/**
 * @brief Render time and date on a 2-line LCD.
 * @details
 * Description:
 *   Prints HH:MM:SS on the first line and a scrolling date string on the second line.
 *
 * @param lcd Reference to the LCD instance used for output.
 * @param hours Hour value (0..23).
 * @param minutes Minute value (0..59).
 * @param seconds Second value (0..59).
 * @param day Day of month (1..31).
 * @param month Month (1..12).
 * @param year Full year (e.g., 2026).
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void printTimeDateOnScreen(
    LiquidCrystal& lcd,
    uint8_t hours,
    uint8_t minutes,
    uint8_t seconds,
    uint8_t day,
    uint8_t month,
    uint16_t year);

/**
 * @brief Display text on a 2-line LCD with basic paging or scrolling.
 * @details
 * Description:
 *   Short text is shown on the first line. Medium text is split across both lines.
 *   Longer text scrolls across the two lines to reveal the full message.
 *
 * @param lcd Reference to the LCD instance used for output.
 * @param text Text to display and/or scroll.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void displayLongText(LiquidCrystal& lcd, String text);

/**
 * @brief Set the SPI chip select pin used by DABSpiMsg.
 * @details
 * Description:
 *   Stores the DAB SPI select pin for use by the DAB library callback.
 *
 * @param pin SPI chip select pin assigned to the DAB shield.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void setDabSpiSelectPin(uint8_t pin);

/**
 * @brief DABShield SPI message callback.
 * @details
 * Description:
 *   Implements the SPI transfer required by the DAB library. Call
 *   setDabSpiSelectPin() before first use.
 *
 * @param data Pointer to the SPI buffer.
 * @param len Length of the buffer in bytes.
 *
 * @author GOLETTA David
 * @date 11/02/2026
 */
void DABSpiMsg(unsigned char* data, uint32_t len);
