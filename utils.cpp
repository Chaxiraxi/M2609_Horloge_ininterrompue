#include "utils.h"

void printDabTime(DABTime& dabtime, Notification& notifier) {
    notifier.debug("Local Time (DAB): " +
                   String(dabtime.Days) + "/" +
                   String(dabtime.Months) + "/" +
                   String(dabtime.Year) + " " +
                   formatTwoDigits(dabtime.Hours) + ":" +
                   formatTwoDigits(dabtime.Minutes) + ":" +
                   formatTwoDigits(dabtime.Seconds));
}

void printGpsTime(Adafruit_GPS& GPS, Notification& notifier) {
    String milliseconds = String(GPS.milliseconds);
    if (GPS.milliseconds < 10) {
        milliseconds = "00" + milliseconds;
    } else if (GPS.milliseconds < 100) {
        milliseconds = "0" + milliseconds;
    }

    notifier.debug("UTC Time (GPS): " +
                   String(GPS.day) + "/" +
                   String(GPS.month) + "/" +
                   String(GPS.year) + " " +
                   formatTwoDigits(GPS.hour) + ":" +
                   formatTwoDigits(GPS.minute) + ":" +
                   formatTwoDigits(GPS.seconds) + "." +
                   milliseconds);
}

// clang-format off
String dateToDayOfTheWeek(uint16_t year, uint8_t month, uint8_t day, bool shortForm) {
    // Zeller's Congruence algorithm
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    uint16_t k = year % 100;
    uint16_t j = year / 100;
    uint8_t h = (day + (13 * (month + 1)) / 5 + k + (k / 4) + (j / 4) - (2 * j)) % 7;

    switch (h) {
        case 0: return shortForm ? "Sat" : "Saturday";
        case 1: return shortForm ? "Sun" : "Sunday";
        case 2: return shortForm ? "Mon" : "Monday";
        case 3: return shortForm ? "Tue" : "Tuesday";
        case 4: return shortForm ? "Wed" : "Wednesday";
        case 5: return shortForm ? "Thu" : "Thursday";
        case 6: return shortForm ? "Fri" : "Friday";
        default: return "";
    }
}
// clang-format on

String formatTwoDigits(uint8_t value) {
    return (value < 10 ? "0" : "") + String(value);
}

void printTimeDateOnScreen(LiquidCrystal* lcd, uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month,
                           uint16_t year) {
    lcd->setCursor(0, 0);
    lcd->print(formatTwoDigits(hours));
    lcd->print(':');
    lcd->print(formatTwoDigits(minutes));
    lcd->print(':');
    lcd->print(formatTwoDigits(seconds));
    lcd->print("        ");

    // Manual scrolling implementation for second line
    String dayOfWeek = dateToDayOfTheWeek(year, month, day, true);
    lcd->setCursor(0, 1);
    static uint32_t lastScrollTime = 0;
    static uint8_t scrollOffset = 0;
    static const uint16_t SCROLL_DELAY_MS = 500;

    String scrollText = dayOfWeek + " " + (day < 10 ? "0" : "") + String(day) + "/" +
                        (month < 10 ? "0" : "") + String(month) + "/" +
                        (year < 10 ? "000" : year < 100 ? "00"
                                         : year < 1000  ? "0"
                                                        : "") +
                        String(year) + "  ";

    lcd->print(" ");
    if (millis() - lastScrollTime >= SCROLL_DELAY_MS) {
        lastScrollTime = millis();
        scrollOffset = (scrollOffset + 1) % scrollText.length();
    }

    for (uint8_t i = 0; i < 15; i++) {
        lcd->print(scrollText[(scrollOffset + i) % scrollText.length()]);
    }
}

void displayLongText(LiquidCrystal* lcd, const String& text) {
    // If shorter than 8, print on the first line
    // If between 8 and 16 chars, split and print on both lines
    // If longer than 16 chars, split after 8 char on the second line and scroll both lines together to read the full text
    if (text.length() <= 8) {
        lcd->setCursor(0, 0);
        lcd->print(text);
    } else if (text.length() <= 16) {
        lcd->setCursor(0, 0);
        lcd->print(text.substring(0, 8));
        lcd->setCursor(0, 1);
        lcd->print(text.substring(8));
    } else {
        // For longer text, we can implement a scrolling mechanism similar to the date line in printTimeDateOnScreen
        static uint32_t lastScrollTime = 0;
        static uint8_t scrollOffset = 0;
        static const uint16_t SCROLL_DELAY_MS = 500;
        String scrollText = text + "  ";  // Add some spacing at the end for better readability when scrolling
        if (millis() - lastScrollTime >= SCROLL_DELAY_MS) {
            lastScrollTime = millis();
            scrollOffset = (scrollOffset + 1) % scrollText.length();
        }
        lcd->setCursor(0, 0);
        for (uint8_t i = 0; i < 8; i++) {
            lcd->print(scrollText[(scrollOffset + i) % scrollText.length()]);
        }
        lcd->setCursor(0, 1);
        for (uint8_t i = 0; i < 8; i++) {
            lcd->print(scrollText[(scrollOffset + 8 + i) % scrollText.length()]);
        }
    }
}