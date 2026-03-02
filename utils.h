#pragma once

#include <Adafruit_GPS.h>
#include <Arduino.h>
#include <DABShield.h>
#include <LiquidCrystalIO.h>

#include "Notification.h"

/**
 * @brief Print a formatted local date/time from DAB to the serial console.
 * @details
 * Description:
 *   Outputs DAB-provided date and time in the format DD/MM/YYYY HH:MM:SS.
 *
 * @param dabtime Reference to the DABTime structure containing date/time fields.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
void printDabTime(DABTime& dabtime, Notification& notifier);

/**
 * @brief Print a formatted UTC date/time from GPS to the serial console.
 * @details
 * Description:
 *   Outputs GPS-provided UTC date and time in the format DD/MM/YY HH:MM:SS.mmm.
 *
 * @param GPS Reference to the Adafruit_GPS object containing parsed date/time fields.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
void printGpsTime(Adafruit_GPS& GPS, Notification& notifier);

/**
 * @brief Convert a calendar date to its day-of-week name.
 * @details
 * Description:
 *   Uses Zeller's Congruence to compute the weekday for a given date.
 *   Returns either full or abbreviated weekday names.
 *
 * @param year Full year (e.g., 2026).
 * @param month Month in range [1..12].
 * @param day Day in range [1..31].
 * @param shortForm True to return abbreviated names (e.g., "Mon"), false for full names (e.g., "Monday").
 * @return Weekday as an Arduino String. Returns an empty string if computation falls outside expected range.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
String dateToDayOfTheWeek(uint16_t year, uint8_t month, uint8_t day, bool shortForm = false);

/**
 * @brief Format a value as a two-digit string.
 * @details
 * Description:
 *   Converts a numeric value to a string with at least two digits, adding a leading zero if necessary.
 *
 * @param value Numeric value to format.
 * @return Formatted two-digit string.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
String formatTwoDigits(uint8_t value);

/**
 * @brief Print time and date on the LCD.
 * @details
 * Description:
 *   Writes HH:MM:SS on the first line and scrolls the day/date on the second line.
 *
 * @param lcd Pointer to the LCD instance.
 * @param hours Hours in range [0..23].
 * @param minutes Minutes in range [0..59].
 * @param seconds Seconds in range [0..59].
 * @param day Day in range [1..31].
 * @param month Month in range [1..12].
 * @param year Full year (e.g., 2026).
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
void printTimeDateOnScreen(LiquidCrystal* lcd, uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month,
                           uint16_t year);

/**
 * @brief Display text on the LCD with optional scrolling.
 * @details
 * Description:
 *   Prints short text as-is, splits medium text across two lines,
 *   and scrolls longer text across both lines.
 *
 * @param lcd Pointer to the LCD instance.
 * @param text Text to display.
 *
 * @author GOLETTA David
 * @date 12/02/2026
 */
void displayLongText(LiquidCrystal* lcd, const String& text);