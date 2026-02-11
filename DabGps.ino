#include <Adafruit_GPS.h>
#include <DABShield.h>
#include <IoAbstractionWire.h>
#include <LiquidCrystalIO.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "DABTimeSource.h"
#include "GPSTimeSource.h"
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
const int lcdRs = 8;
const int lcdEn = 9;
const int lcdD4 = 10;
const int lcdD5 = 11;
const int lcdD6 = 12;
const int lcdD7 = 13;
const int resetPin23017 = 5;

LiquidCrystal lcd(lcdRs, lcdEn, lcdD4, lcdD5, lcdD6, lcdD7, ioFrom23017(0x20));

DABTimeSource dabTimeSource(Dab, dabtime, hasService, TIMEZONE_OFFSET_HOURS);
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

void printTimeDateOnScreen(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month, uint16_t year) {
    lcd.setCursor(0, 0);
    printTwoDigits(hours);
    lcd.print(':');
    printTwoDigits(minutes);
    lcd.print(':');
    printTwoDigits(seconds);
    lcd.print("        ");

    lcd.setCursor(0, 1);
    printTwoDigits(day);
    lcd.print('/');
    printTwoDigits(month);
    lcd.print('/');
    if (year < 1000) {
        lcd.print('0');
        if (year < 100) {
            lcd.print('0');
            if (year < 10) {
                lcd.print('0');
            }
        }
    }
    lcd.print(year);
    lcd.print("      ");
}

void setup() {
    // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
    // also spit it out
    Serial.begin(115200);
    Serial.println("Adafruit GPS library basic parsing test!");

    // Reset the MCP23017 I/O expander to ensure it's in a known state (it can get into a bad state if power is removed while it's writing to its registers)
    pinMode(resetPin23017, OUTPUT);
    digitalWrite(resetPin23017, LOW);
    delayMicroseconds(100);
    digitalWrite(resetPin23017, HIGH);

    Wire.begin();
    lcd.begin(16, 2);
    analogWriteResolution(12);
    analogWrite(SCREEN_CONTRAST_PIN, 600);

    gpsTimeSource.init();
    dabTimeSource.init(dabSpiSelectPin, SPEAKER_OUTPUT);
}

uint32_t timer = millis();
void loop() {
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if ((c) && (GPSECHO)) Serial.write(c);

    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
        // a tricky thing here is if we print the NMEA sentence, or data
        // we end up not listening and catching other sentences!
        // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
        // Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

        if (!GPS.parse(GPS.lastNMEA()))  // this also sets the newNMEAreceived() flag to false
            return;                      // we can fail to parse a sentence in which case we should just wait for another
    }

    // approximately every 500 ms, print both GPS and DAB times
    if (millis() - timer > 1000) {
        timer = millis();

        // printGpsTime();

        DateTimeFields dateTime;
        if (dabTimeSource.getDateTime(dateTime)) {
            printTimeDateOnScreen(dateTime.time.hour, dateTime.time.minute, dateTime.time.second, dateTime.date.day, dateTime.date.month, dateTime.date.year);
        } else if (gpsTimeSource.getDateTime(dateTime)) {
            printTimeDateOnScreen(dateTime.time.hour, dateTime.time.minute, dateTime.time.second, dateTime.date.day, dateTime.date.month, dateTime.date.year);
        }
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