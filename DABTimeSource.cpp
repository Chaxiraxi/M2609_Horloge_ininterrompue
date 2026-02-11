#include "DABTimeSource.h"

#include <SPI.h>

DABTimeSource::DABTimeSource(DAB& dab, DABTime& dabtime, bool& hasService, int8_t timezoneOffsetHours)
    : dab_(dab), dabtime_(dabtime), hasService_(hasService), timezoneOffsetHours_(timezoneOffsetHours) {}

bool DABTimeSource::init(uint8_t spiSelectPin, uint8_t speakerOutput, uint8_t band) {
    pinMode(spiSelectPin, OUTPUT);
    digitalWrite(spiSelectPin, HIGH);
    SPI.begin();

    Serial.println(F("Initializing DAB Shield..."));
    dab_.speaker(static_cast<DABSpeaker>(speakerOutput));
    dab_.begin(band);

    if (dab_.error != 0) {
        Serial.print(F("ERROR: "));
        Serial.println(dab_.error);
        Serial.println(F("Check DABShield connection and SPI"));
        return false;
    }

    Serial.println(F("Scanning for DAB services..."));
    hasService_ = tuneFirstAvailableService();
    if (!hasService_) {
        Serial.println(F("No DAB services found."));
    }

    return hasService_;
}

bool DABTimeSource::tuneFirstAvailableService() {
    for (uint8_t freq_index = 0; freq_index < DAB_FREQS; freq_index++) {
        dab_.tune(freq_index);
        if (dab_.servicevalid() == true) {
            tunedServiceIndex_ = 0;
            dab_.set_service(tunedServiceIndex_);
            Serial.print(F("Tuned ensemble: "));
            Serial.println(dab_.Ensemble);
            Serial.print(F("Service: "));
            Serial.println(dab_.service[tunedServiceIndex_].Label);
            return true;
        }
    }
    return false;
}

bool DABTimeSource::isSaneDateTime() const {
    if (dabtime_.Year == 0 || dabtime_.Months == 0 || dabtime_.Days == 0) return false;
    if (dabtime_.Months > 12 || dabtime_.Days > 31) return false;
    if (dabtime_.Hours > 23 || dabtime_.Minutes > 59 || dabtime_.Seconds > 59) return false;
    return true;
}

bool DABTimeSource::getDateTime(DateTimeFields& out) {
    if (!isEnabled()) return false;

    static uint32_t lastStatusCheckMs = 0;
    if (millis() - lastStatusCheckMs < 1000) return isSaneDateTime();  // avoid querying DAB status too frequently
    lastStatusCheckMs = millis();

    if (!hasService_) {
        Serial.println(F("Local Time (DAB): no service"));
        return false;
    }

    hasService_ = dab_.status() == 1;
    if (!hasService_) {
        Serial.println(F("Local Time (DAB): service lost"));
        return false;
    }

    if (dab_.time(&dabtime_) != 0) {
        Serial.println(F("Local Time (DAB): unavailable"));
        return false;
    }

    if (!isSaneDateTime()) return false;

    out.date.year = dabtime_.Year;
    out.date.month = dabtime_.Months;
    out.date.day = dabtime_.Days;

    out.time.hour = wrapHours(static_cast<int32_t>(dabtime_.Hours) + timezoneOffsetHours_);
    out.time.minute = dabtime_.Minutes;
    out.time.second = dabtime_.Seconds;

    return true;
}
