#include <Adafruit_GPS.h>
#include <DABShield.h>
#include <IoAbstractionWire.h>
#include <LiquidCrystalIO.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial gpsSerial(3, 4);
Adafruit_GPS GPS(&gpsSerial);

#define GPSECHO false
#define SPEAKER_OUTPUT SPEAKER_NONE  // SPEAKER_NONE, SPEAKER_DIFF, SPEAKER_STEREO
#define SCREEN_CONTRAST_PIN A0

DAB Dab;
DABTime dabtime;
bool hasService = false;
const byte dabSpiSelectPin = 8;
uint8_t tunedServiceIndex = 0;
uint32_t lastTimePrintMs = 0;
const int lcdRs = 8;
const int lcdEn = 9;
const int lcdD4 = 10;
const int lcdD5 = 11;
const int lcdD6 = 12;
const int lcdD7 = 13;
const int resetPin23017 = 5;

LiquidCrystal lcd(lcdRs, lcdEn, lcdD4, lcdD5, lcdD6, lcdD7, ioFrom23017(0x20));

bool tuneFirstAvailableService() {
    for (uint8_t freq_index = 0; freq_index < DAB_FREQS; freq_index++) {
        Dab.tune(freq_index);
        if (Dab.servicevalid() == true) {
            tunedServiceIndex = 0;
            Dab.set_service(tunedServiceIndex);
            Serial.print(F("Tuned ensemble: "));
            Serial.println(Dab.Ensemble);
            Serial.print(F("Service: "));
            Serial.println(Dab.service[tunedServiceIndex].Label);
            return true;
        }
    }
    return false;
}

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

uint8_t wrapHours(int32_t hours) {
    while (hours < 0) {
        hours += 24;
    }
    while (hours >= 24) {
        hours -= 24;
    }
    return static_cast<uint8_t>(hours);
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

void debugStartupBlink(int pin = 13, int times = 3, int delayMs = 500) {
    pinMode(pin, OUTPUT);
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayMs);
        digitalWrite(pin, LOW);
        delay(delayMs);
    }
}

void setup() {
    // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
    // also spit it out
    debugStartupBlink();
    Serial.begin(115200);
    delay(5000);
    debugStartupBlink();
    Serial.println("Adafruit GPS library basic parsing test!");
    pinMode(resetPin23017, OUTPUT);
    digitalWrite(resetPin23017, LOW);
    delayMicroseconds(100);
    digitalWrite(resetPin23017, HIGH);

    Wire.begin();
    lcd.begin(16, 2);
    analogReadResolution(12);
    analogWriteResolution(12);
    analogWrite(SCREEN_CONTRAST_PIN, 600);

    // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
    GPS.begin(9600);

    // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // uncomment this line to turn on only the "minimum recommended" data
    // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
    // the parser doesn't care about other sentences at this time

    // Set the update rate
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);  // 1 Hz update rate
    // For the parsing code to work nicely and have time to sort thru the data, and
    // print it out we don't suggest using anything higher than 1 Hz

    // Request updates on antenna status, comment out to keep quiet
    GPS.sendCommand(PGCMD_ANTENNA);

    delay(1000);
    // Ask for firmware version
    gpsSerial.println(PMTK_Q_RELEASE);

    pinMode(dabSpiSelectPin, OUTPUT);
    digitalWrite(dabSpiSelectPin, HIGH);
    SPI.begin();

    Serial.println(F("Initializing DAB Shield..."));
    Dab.speaker(SPEAKER_OUTPUT);
    Dab.begin(0);  // 0 = DAB band, 1 = FM band

    if (Dab.error != 0) {
        Serial.print(F("ERROR: "));
        Serial.println(Dab.error);
        Serial.println(F("Check DABShield connection and SPI"));
        return;
    }

    Serial.println(F("Scanning for DAB services..."));
    hasService = tuneFirstAvailableService();
    if (!hasService) {
        Serial.println(F("No DAB services found."));
    }
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

        bool displayed = false;

        if (hasService) {
            hasService = Dab.status();
            if (hasService) {
                bool notError = Dab.time(&dabtime) == 0;
                if (notError) {
                    // printDabTime();
                    constexpr int8_t TIMEZONE_OFFSET_HOURS = 1;  // Adjust this value based on your local timezone
                    uint8_t localHours = wrapHours(static_cast<int32_t>(dabtime.Hours) + TIMEZONE_OFFSET_HOURS);
                    printTimeDateOnScreen(localHours, dabtime.Minutes, dabtime.Seconds, dabtime.Days, dabtime.Months, dabtime.Year);
                    displayed = true;
                } else {
                    Serial.println(F("Local Time (DAB): unavailable"));
                }
            } else {
                Serial.println(F("Local Time (DAB): service lost"));
            }
        } else {
            Serial.println(F("Local Time (DAB): no service"));
        }
        if (!displayed) {
            constexpr int8_t TIMEZONE_OFFSET_HOURS = 1;  // Adjust this value based on your local timezone
            uint8_t localHours = wrapHours(static_cast<int32_t>(GPS.hour) + TIMEZONE_OFFSET_HOURS);
            printTimeDateOnScreen(localHours, GPS.minute, GPS.seconds, GPS.day, GPS.month, 2000 + GPS.year);
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