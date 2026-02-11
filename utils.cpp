#include "utils.h"

#include <SPI.h>

namespace {
uint8_t dabSpiSelectPin = 0;
}  // namespace

void printDabTime(const DABTime& dabtime) {
    Serial.print(F("Local Time (DAB): "));
    Serial.print(dabtime.Days);
    Serial.print('/');
    Serial.print(dabtime.Months);
    Serial.print('/');
    Serial.print(dabtime.Year);
    Serial.print(' ');
    if (dabtime.Hours < 10) {
        Serial.print('0');
    }
    Serial.print(dabtime.Hours);
    Serial.print(':');
    if (dabtime.Minutes < 10) {
        Serial.print('0');
    }
    Serial.print(dabtime.Minutes);
    Serial.print(':');
    if (dabtime.Seconds < 10) {
        Serial.print('0');
    }
    Serial.println(dabtime.Seconds);
}

void printGpsTime(const Adafruit_GPS& gps) {
    Serial.print("\nUTC Time (GPS): ");
    Serial.print(gps.day);
    Serial.print('/');
    Serial.print(gps.month);
    Serial.print('/');
    Serial.print(gps.year);
    Serial.print(' ');

    if (gps.hour < 10) {
        Serial.print('0');
    }
    Serial.print(gps.hour, DEC);
    Serial.print(':');
    if (gps.minute < 10) {
        Serial.print('0');
    }
    Serial.print(gps.minute, DEC);
    Serial.print(':');
    if (gps.seconds < 10) {
        Serial.print('0');
    }
    Serial.print(gps.seconds, DEC);
    Serial.print('.');
    if (gps.milliseconds < 10) {
        Serial.print("00");
    } else if (gps.milliseconds > 9 && gps.milliseconds < 100) {
        Serial.print("0");
    }
    Serial.println(gps.milliseconds);
}

void printTwoDigits(LiquidCrystal& lcd, uint8_t value) {
    if (value < 10) {
        lcd.print('0');
    }
    lcd.print(value, DEC);
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

void printTimeDateOnScreen(
    LiquidCrystal& lcd,
    uint8_t hours,
    uint8_t minutes,
    uint8_t seconds,
    uint8_t day,
    uint8_t month,
    uint16_t year) {
    lcd.setCursor(0, 0);
    printTwoDigits(lcd, hours);
    lcd.print(':');
    printTwoDigits(lcd, minutes);
    lcd.print(':');
    printTwoDigits(lcd, seconds);
    lcd.print("        ");

    // Manual scrolling implementation for second line
    String dayOfWeek = dateToDayOfTheWeek(year, month, day, true);
    lcd.setCursor(0, 1);
    static uint32_t lastScrollTime = 0;
    static uint8_t scrollOffset = 0;
    static const uint16_t SCROLL_DELAY_MS = 500;

    String scrollText = dayOfWeek + " " + (day < 10 ? "0" : "") + String(day) + "/" +
                        (month < 10 ? "0" : "") + String(month) + "/" +
                        (year < 10 ? "000" : year < 100 ? "00"
                                         : year < 1000  ? "0"
                                                        : "") +
                        String(year) + "  ";

    lcd.print(" ");
    if (millis() - lastScrollTime >= SCROLL_DELAY_MS) {
        lastScrollTime = millis();
        scrollOffset = (scrollOffset + 1) % scrollText.length();
    }

    for (uint8_t i = 0; i < 15; i++) {
        lcd.print(scrollText[(scrollOffset + i) % scrollText.length()]);
    }
}

void displayLongText(LiquidCrystal& lcd, String text) {
    // If shorter than 8, print on the first line
    // If between 8 and 16 chars, split and print on both lines
    // If longer than 16 chars, split after 8 char on the second line and scroll both lines together to read the full text
    if (text.length() <= 8) {
        lcd.setCursor(0, 0);
        lcd.print(text);
    } else if (text.length() <= 16) {
        lcd.setCursor(0, 0);
        lcd.print(text.substring(0, 8));
        lcd.setCursor(0, 1);
        lcd.print(text.substring(8));
    } else {
        // For longer text, we can implement a scrolling mechanism similar to the date line in printTimeDateOnScreen
        static uint32_t lastScrollTime = 0;
        static uint8_t scrollOffset = 0;
        static const uint16_t SCROLL_DELAY_MS = 500;
        text += "  ";  // Add some spacing at the end for better readability when scrolling
        if (millis() - lastScrollTime >= SCROLL_DELAY_MS) {
            lastScrollTime = millis();
            scrollOffset = (scrollOffset + 1) % text.length();
        }
        lcd.setCursor(0, 0);
        for (uint8_t i = 0; i < 8; i++) {
            lcd.print(text[(scrollOffset + i) % text.length()]);
        }
        lcd.setCursor(0, 1);
        for (uint8_t i = 0; i < 8; i++) {
            lcd.print(text[(scrollOffset + 8 + i) % text.length()]);
        }
    }
}

void setDabSpiSelectPin(uint8_t pin) {
    dabSpiSelectPin = pin;
}

// Hardware SPI
void DABSpiMsg(unsigned char* data, uint32_t len) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(dabSpiSelectPin, LOW);
    SPI.transfer(data, len);
    digitalWrite(dabSpiSelectPin, HIGH);
    SPI.endTransaction();
}
