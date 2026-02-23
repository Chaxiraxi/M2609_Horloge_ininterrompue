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
#include "NTPTimeSource.h"
#include "Notification.h"
#include "PinDefinitions.h"
#include "TimeCoordinator.h"
#include "TimeSource.h"
#include "UiController.h"
#include "WifiManager.h"
#include "utils.h"

SoftwareSerial gpsSerial(3, 4);
Adafruit_GPS GPS(&gpsSerial);

#define GPSECHO false
#define SPEAKER_OUTPUT SPEAKER_NONE  // SPEAKER_NONE, SPEAKER_DIFF, SPEAKER_STEREO

DAB Dab;
DABTime dabtime;
bool hasService = false;
constexpr int8_t TIMEZONE_OFFSET_HOURS = 1;
const byte dabSpiSelectPin = 8;

Notification Logger;
SerialTransport serialTransport(115200);
WiFiManager wifiManager(Logger);
IoAbstractionRef mcpIo = ioFrom23017(0x20);

Button setButton(SET_BTN);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mcpIo);

DABTimeSource dabTimeSource(Dab, dabtime, hasService, 0, &Logger);
GPSTimeSource gpsTimeSource(GPS, TIMEZONE_OFFSET_HOURS);
NTPTimeSource ntpTimeSource("pool.ntp.org", TIMEZONE_OFFSET_HOURS * 3600, &Logger);

// Sources array in priority order: DAB > NTP > GPS
TimeSource* timeSources[] = {&dabTimeSource, &ntpTimeSource, &gpsTimeSource};
constexpr uint8_t TIME_SOURCE_COUNT = sizeof(timeSources) / sizeof(timeSources[0]);

TimeCoordinator coordinator(&Logger);
UiController ui(lcd, setButton, coordinator, timeSources, TIME_SOURCE_COUNT, mcpIo, &Logger);

#define DEBUG_MODE true

void setup() {
    // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
    // also spit it out
    serialTransport.init();
    Logger.addTransport(&serialTransport, DEBUG_MODE ? Notification::DEBUG : Notification::INFO);

    // Reset the MCP23017 I/O expander to ensure it's in a known state (it can get into a bad state if power is removed while it's writing to its registers)
    pinMode(RESET_PIN_23017, OUTPUT);
    digitalWrite(RESET_PIN_23017, LOW);
    delayMicroseconds(100);
    digitalWrite(RESET_PIN_23017, HIGH);

    Wire.begin();
    lcd.begin(8, 2);
    analogWriteResolution(12);
    analogWrite(SCREEN_CONTRAST_PIN, 600);

    displayLongText(&lcd, "Connecting WiFi");
    wifiManager.connectToWiFi();
    ntpTimeSource.init();
    gpsTimeSource.init();
    displayLongText(&lcd, "Initializing DAB");
    dabTimeSource.init(dabSpiSelectPin, SPEAKER_OUTPUT);

    coordinator.setSources(timeSources, TIME_SOURCE_COUNT);
    ui.begin();
}

void loop() {
    // Drain GPS bytes as fast as possible. Reading only one byte per loop
    // iteration can fall behind when the loop does slower work (LCD/I2C).
    while (gpsSerial.available() > 0) {
        char c = GPS.read();
        if (GPSECHO && c) {
            Serial.write(c);
        }
    }

    // Parse any completed NMEA sentences.
    while (GPS.newNMEAreceived()) {
        GPS.parse(GPS.lastNMEA());
    }

    // Run the time coordinator (queries sources, coherence check, software clock).
    coordinator.update();

    // Run the UI controller (reads buttons/encoder, manages modes, refreshes LCD).
    ui.update();
}

// Hardware SPI
void DABSpiMsg(unsigned char* data, uint32_t len) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(dabSpiSelectPin, LOW);
    SPI.transfer(data, len);
    digitalWrite(dabSpiSelectPin, HIGH);
    SPI.endTransaction();
}