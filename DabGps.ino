#include <Adafruit_GPS.h>
#include <DABShield.h>
#include <IoAbstractionWire.h>
#include <LiquidCrystalIO.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "src/core/logging/Notification.hpp"
#include "src/network/RestApiServer.hpp"
#include "src/network/WifiManager.hpp"
#include "src/platform/PinDefinitions.hpp"
#include "src/time/core/TimeCoordinator.hpp"
#include "src/time/core/TimeSource.hpp"
#include "src/time/sources/DABTimeSource.hpp"
#include "src/time/sources/GPSTimeSource.hpp"
#include "src/time/sources/NTPTimeSource.hpp"
#include "src/ui/UiController.hpp"
#include "src/ui/input/Button.hpp"
#include "src/utils/utils.hpp"

SoftwareSerial gpsSerial = SoftwareSerial(3, 4);
Adafruit_GPS GPS = Adafruit_GPS(&gpsSerial);

#define GPSECHO false
#define SPEAKER_OUTPUT SPEAKER_NONE  // SPEAKER_NONE, SPEAKER_DIFF, SPEAKER_STEREO

DAB Dab;
DABTime dabtime;
bool hasService = false;
constexpr int8_t TIMEZONE_OFFSET_HOURS = 1;
const byte dabSpiSelectPin = 8;

Notification Logger;
SerialTransport serialTransport = SerialTransport(115200);
WiFiManager wifiManager = WiFiManager(Logger);
IoAbstractionRef mcpIo = ioFrom23017(0x20);

Button setButton = Button(SET_BTN);
LiquidCrystal lcd = LiquidCrystal(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, mcpIo);

DABTimeSource dabTimeSource = DABTimeSource(Dab, dabtime, hasService, 0, &Logger);
GPSTimeSource gpsTimeSource = GPSTimeSource(GPS, TIMEZONE_OFFSET_HOURS);
NTPTimeSource ntpTimeSource = NTPTimeSource("pool.ntp.org", TIMEZONE_OFFSET_HOURS * 3600, &Logger);

// Sources array in priority order: DAB > NTP > GPS
TimeSource* timeSources[] = {&dabTimeSource, &ntpTimeSource, &gpsTimeSource};
constexpr uint8_t TIME_SOURCE_COUNT = sizeof(timeSources) / sizeof(timeSources[0]);

TimeCoordinator coordinator = TimeCoordinator(&Logger);
RestApiServer restApiServer = RestApiServer(coordinator, timeSources, TIME_SOURCE_COUNT, &Logger);
UiController ui = UiController(lcd, setButton, coordinator, timeSources, TIME_SOURCE_COUNT, mcpIo, &Logger);

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
    restApiServer.begin();
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

    // Serve REST API + embedded web page.
    restApiServer.update();
}

/**
 * @brief SPI message handler for DAB communication.
 * @details
 * This function is called by the DAB library when it needs to send a message over SPI.
 * This function is required by the DAB Library (externally referenced), and we must define it ourselves to specify how SPI communication should be performed with the DAB module.
 *
 * @param data Pointer to the data buffer to send.
 * @param len Length of the data buffer.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 */
void DABSpiMsg(unsigned char* data, uint32_t len) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(dabSpiSelectPin, LOW);
    SPI.transfer(data, len);
    digitalWrite(dabSpiSelectPin, HIGH);
    SPI.endTransaction();
}