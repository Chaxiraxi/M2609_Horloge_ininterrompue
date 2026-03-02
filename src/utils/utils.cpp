#include "utils.hpp"

namespace {
/**
 * @internal
 * @brief Normalize text to one 8-character LCD line.
 * @details
 * Truncates text longer than 8 characters and right-pads shorter text with spaces.
 *
 * @param input Raw text content.
 * @return Normalized 8-character LCD line.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
String normalizeLcdLine(const String& input) {
    if (input.length() == 8) return input;
    if (input.length() > 8) return input.substring(0, 8);

    String out = input;
    while (out.length() < 8) out += ' ';
    return out;
}

/**
 * @internal
 * @brief Write an LCD row only when content changes.
 * @details
 * Maintains a per-row cache and sends I2C LCD writes only when the normalized text differs,
 * reducing unnecessary bus traffic.
 *
 * @param lcd Pointer to target LCD instance.
 * @param row LCD row index (0 or 1).
 * @param text Text to display on the selected row.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void writeLcdLineIfChanged(LiquidCrystal* lcd, uint8_t row, const String& text) {
    static String lastLine0 = "";
    static String lastLine1 = "";

    String normalized = normalizeLcdLine(text);
    String* last = (row == 0) ? &lastLine0 : &lastLine1;
    if (*last == normalized) return;

    lcd->setCursor(0, row);
    lcd->print(normalized);
    *last = normalized;
}
}  // namespace

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
    String line0 = formatTwoDigits(hours) + ":" + formatTwoDigits(minutes) + ":" + formatTwoDigits(seconds);
    writeLcdLineIfChanged(lcd, 0, line0);

    // Manual scrolling implementation for second line
    String dayOfWeek = dateToDayOfTheWeek(year, month, day, true);
    static uint32_t lastScrollTime = 0;
    static uint8_t scrollOffset = 0;
    static const uint16_t SCROLL_DELAY_MS = 500;

    String scrollText = dayOfWeek + " " + (day < 10 ? "0" : "") + String(day) + "/" +
                        (month < 10 ? "0" : "") + String(month) + "/" +
                        (year < 10 ? "000" : year < 100 ? "00"
                                         : year < 1000  ? "0"
                                                        : "") +
                        String(year) + "  ";

    if (millis() - lastScrollTime >= SCROLL_DELAY_MS) {
        lastScrollTime = millis();
        scrollOffset = (scrollOffset + 1) % scrollText.length();
    }

    String line1 = " ";
    for (uint8_t i = 0; i < 7; i++) {
        line1 += scrollText[(scrollOffset + i) % scrollText.length()];
    }
    writeLcdLineIfChanged(lcd, 1, line1);
}

void displayLongText(LiquidCrystal* lcd, const String& text) {
    // If shorter than 8, print on the first line
    // If between 8 and 16 chars, split and print on both lines
    // If longer than 16 chars, split after 8 char on the second line and scroll both lines together to read the full text
    if (text.length() <= 8) {
        writeLcdLineIfChanged(lcd, 0, text);
        writeLcdLineIfChanged(lcd, 1, "");
    } else if (text.length() <= 16) {
        writeLcdLineIfChanged(lcd, 0, text.substring(0, 8));
        writeLcdLineIfChanged(lcd, 1, text.substring(8));
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
        String line0 = "";
        for (uint8_t i = 0; i < 8; i++) {
            line0 += scrollText[(scrollOffset + i) % scrollText.length()];
        }
        String line1 = "";
        for (uint8_t i = 0; i < 8; i++) {
            line1 += scrollText[(scrollOffset + 8 + i) % scrollText.length()];
        }
        writeLcdLineIfChanged(lcd, 0, line0);
        writeLcdLineIfChanged(lcd, 1, line1);
    }
}