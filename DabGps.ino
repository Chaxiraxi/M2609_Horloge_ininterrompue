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
#include "utils.h"

SoftwareSerial gpsSerial(3, 4);
Adafruit_GPS GPS(&gpsSerial);

#define GPSECHO false
#define SPEAKER_OUTPUT SPEAKER_NONE  // SPEAKER_NONE, SPEAKER_DIFF, SPEAKER_STEREO
#define SCREEN_CONTRAST_PIN A0

DAB Dab;
DABTime dabtime;
bool hasService = false;
constexpr int8_t TIMEZONE_OFFSET_HOURS = 1;
uint32_t lastTimePrintMs = 0;

Button setButton(SET_BTN);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, ioFrom23017(0x20));

DABTimeSource dabTimeSource(Dab, dabtime, hasService);
GPSTimeSource gpsTimeSource(GPS, TIMEZONE_OFFSET_HOURS);

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

    setDabSpiSelectPin(DAB_SPI_SELECT_PIN);
    gpsTimeSource.init();
    dabTimeSource.init(DAB_SPI_SELECT_PIN, SPEAKER_OUTPUT);

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
            printTimeDateOnScreen(lcd, dateTime.time.hour, dateTime.time.minute, dateTime.time.second, dateTime.date.day, dateTime.date.month, dateTime.date.year);
        } else if (gpsTimeSource.getDateTime(dateTime)) {
            printTimeDateOnScreen(lcd, dateTime.time.hour, dateTime.time.minute, dateTime.time.second, dateTime.date.day, dateTime.date.month, dateTime.date.year);
        } else {
            displayLongText(lcd, "No time source found");
        }
    } else {
        // Keep looping fast to avoid falling behind on GPS bytes.
    }
}
