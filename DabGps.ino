#include <Adafruit_GPS.h>
#include <DABShield.h>
#include <IoAbstractionWire.h>
#include <LiquidCrystalIO.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "Button.h"
#include "DABTimeSource.h"
#include "GPSTimeSource.h"
#include "PinDefinitions.h"
#include "TimeSource.h"

SoftwareSerial gpsSerial(3, 4);
Adafruit_GPS GPS(&gpsSerial);

#define GPSECHO false
#define SPEAKER_OUTPUT SPEAKER_NONE  // SPEAKER_NONE, SPEAKER_DIFF, SPEAKER_STEREO
#define SCREEN_CONTRAST_PIN A0

DAB Dab;
DABTime dabtime;
bool hasService = false;
constexpr int8_t TIMEZONE_OFFSET_HOURS = 1;
const byte dabSpiSelectPin = 8;
uint32_t lastTimePrintMs = 0;

Button setButton(SET_BTN);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, ioFrom23017(0x20));

DABTimeSource dabTimeSource(Dab, dabtime, hasService);
GPSTimeSource gpsTimeSource(GPS, TIMEZONE_OFFSET_HOURS);

void printDabTime() {
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

void printGpsTime() {
    Serial.print("\nUTC Time (GPS): ");
    Serial.print(GPS.day);
    Serial.print('/');
    Serial.print(GPS.month);
    Serial.print('/');
    Serial.print(GPS.year);
    Serial.print(' ');

    if (GPS.hour < 10) {
        Serial.print('0');
    }
    Serial.print(GPS.hour, DEC);
    Serial.print(':');
    if (GPS.minute < 10) {
        Serial.print('0');
    }
    Serial.print(GPS.minute, DEC);
    Serial.print(':');
    if (GPS.seconds < 10) {
        Serial.print('0');
    }
    Serial.print(GPS.seconds, DEC);
    Serial.print('.');
    if (GPS.milliseconds < 10) {
        Serial.print("00");
    } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
        Serial.print("0");
    }
    Serial.println(GPS.milliseconds);
}

void printTwoDigits(uint8_t value) {
    if (value < 10) {
        lcd.print('0');
    }
    lcd.print(value, DEC);
}

// clang-format off
String dateToDayOfTheWeek(uint16_t year, uint8_t month, uint8_t day, bool shortForm = false) {
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

void printTimeDateOnScreen(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month, uint16_t year) {
    lcd.setCursor(0, 0);
    printTwoDigits(hours);
    lcd.print(':');
    printTwoDigits(minutes);
    lcd.print(':');
    printTwoDigits(seconds);
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

void displayLongText(String text) {
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

void setup() {
    // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
    // also spit it out
    Serial.begin(115200);
    Serial.println("Adafruit GPS library basic parsing test!");

    // Reset the MCP23017 I/O expander to ensure it's in a known state (it can get into a bad state if power is removed while it's writing to its registers)
    pinMode(RESET_PIN_23017, OUTPUT);
    digitalWrite(RESET_PIN_23017, LOW);
    delayMicroseconds(100);
    digitalWrite(RESET_PIN_23017, HIGH);

    Wire.begin();
    lcd.begin(8, 2);
    analogWriteResolution(12);
    analogWrite(SCREEN_CONTRAST_PIN, 600);

    gpsTimeSource.init();
    dabTimeSource.init(dabSpiSelectPin, SPEAKER_OUTPUT);

    // DEBUG
    gpsTimeSource.setEnabled(true);
    dabTimeSource.setEnabled(true);
}

uint32_t timer = millis();
DateTimeFields dateTime;
void loop() {
    setButton.updateState();
    if (setButton.isPressed()) {
        if (dabTimeSource.isEnabled()) {
            dabTimeSource.setEnabled(false);
            Serial.println("DAB time source disabled");
        } else {
            dabTimeSource.setEnabled(true);
            Serial.println("DAB time source enabled");
        }
    }
    // Drain GPS bytes as fast as possible. Reading only one byte per loop
    // iteration can fall behind when the loop does slower work (LCD/I2C).
    while (gpsSerial.available() > 0) {
        char c = GPS.read();
        if (GPSECHO && c) {
            Serial.write(c);
        }
    }

    // If a sentence is received, we can check the checksum, parse it...
    // Use a loop in case we completed more than one sentence while draining bytes.
    while (GPS.newNMEAreceived()) {
        // A tricky thing here is if we print the NMEA sentence, or data
        // we end up not listening and catching other sentences!
        // so be very wary if using OUTPUT_ALLDATA and trying to print out data
        // Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

        if (!GPS.parse(GPS.lastNMEA())) {
            // Parsing failed; keep looping so we continue draining/processing.
            // (A fail can happen if we previously dropped bytes.)
        }
    }

    static uint32_t lastLcdUpdateMs = 0;
    constexpr uint16_t LCD_UPDATE_INTERVAL_MS = 100;
    if (millis() - lastLcdUpdateMs >= LCD_UPDATE_INTERVAL_MS) {
        lastLcdUpdateMs = millis();

        if (dabTimeSource.getDateTime(dateTime)) {
            printTimeDateOnScreen(dateTime.time.hour, dateTime.time.minute, dateTime.time.second, dateTime.date.day, dateTime.date.month, dateTime.date.year);
        } else if (gpsTimeSource.getDateTime(dateTime)) {
            printTimeDateOnScreen(dateTime.time.hour, dateTime.time.minute, dateTime.time.second, dateTime.date.day, dateTime.date.month, dateTime.date.year);
        } else {
            displayLongText("No time source found");
        }
    } else {
        // Keep looping fast to avoid falling behind on GPS bytes.
    }
}

// Hardware SPI
void DABSpiMsg(unsigned char* data, uint32_t len) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(dabSpiSelectPin, LOW);
    SPI.transfer(data, len);
    digitalWrite(dabSpiSelectPin, HIGH);
    SPI.endTransaction();
}