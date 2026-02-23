#include "DABTimeSource.h"

#include <SPI.h>

#include "utils.h"

DABTimeSource::DABTimeSource(DAB& dab, DABTime& dabtime, bool& hasService, int8_t timezoneOffsetHours, Notification* notifier)
    : dab_(dab), dabtime_(dabtime), hasService_(hasService), timezoneOffsetHours_(timezoneOffsetHours), notifier_(notifier) {}

bool DABTimeSource::init(uint8_t spiSelectPin, uint8_t speakerOutput, uint8_t band) {
    pinMode(spiSelectPin, OUTPUT);
    digitalWrite(spiSelectPin, HIGH);
    SPI.begin();

    if (notifier_) {
        notifier_->info("Initializing DAB Shield...");
    }
    dab_.speaker(static_cast<DABSpeaker>(speakerOutput));
    dab_.begin(band);

    if (dab_.error != 0) {
        if (notifier_) {
            notifier_->error("DAB initialization error code: " + String(dab_.error));
            notifier_->error("Check DABShield connection and SPI");
        }
        return false;
    }

    if (notifier_) {
        notifier_->info("Scanning for DAB services...");
    }
    hasService_ = tuneFirstAvailableService();
    if (!hasService_) {
        if (notifier_) {
            notifier_->warning("No DAB services found.");
        }
    }

    return hasService_;
}

bool DABTimeSource::tuneFirstAvailableService() {
    for (uint8_t freq_index = 0; freq_index < DAB_FREQS; freq_index++) {
        dab_.tune(freq_index);
        if (dab_.servicevalid() == true) {
            tunedServiceIndex_ = 0;
            dab_.set_service(tunedServiceIndex_);
            if (notifier_) {
                notifier_->info("Tuned ensemble: " + String(dab_.Ensemble));
                notifier_->info("Service: " + String(dab_.service[tunedServiceIndex_].Label));
            }
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
    static uint32_t statusCheckIntervalMs = 1000;
    if (millis() - lastStatusCheckMs < statusCheckIntervalMs) {
        if (hasCachedDateTime_ && isSaneDateTime()) {
            out = cachedDateTime_;
            return true;
        }
        return false;
    }

    if (isSaneDateTime()) {
        statusCheckIntervalMs = 1000;  // Reset to default interval on success
    } else {
        statusCheckIntervalMs = std::min(statusCheckIntervalMs + 250, 5000UL);  // Increment up to 5000ms
    }
    lastStatusCheckMs = millis();

    if (!hasService_) {
        if (notifier_) {
            notifier_->warning("Local Time (DAB): no service");
        }
        return false;
    }

    hasService_ = dab_.status() == 1;
    if (!hasService_) {
        if (notifier_) {
            notifier_->warning("Local Time (DAB): service lost");
        }
        return false;
    }

    if (dab_.time(&dabtime_) != 0) {
        if (notifier_) {
            notifier_->warning("Local Time (DAB): unavailable");
        }
        return false;
    }

    if (!isSaneDateTime()) {
        if (notifier_) {
            notifier_->error("Local Time (DAB): invalid time received (" +
                             String(dabtime_.Days) + "/" +
                             String(dabtime_.Months) + "/" +
                             String(dabtime_.Year) + " " +
                             formatTwoDigits(dabtime_.Hours) + ":" +
                             formatTwoDigits(dabtime_.Minutes) + ":" +
                             formatTwoDigits(dabtime_.Seconds) + "), waiting for " + String(statusCheckIntervalMs) + "ms before retrying");
        }
        return false;
    }

    out.date.year = dabtime_.Year;
    out.date.month = dabtime_.Months;
    out.date.day = dabtime_.Days;

    out.time.hour = wrapHours(static_cast<int32_t>(dabtime_.Hours) + timezoneOffsetHours_);
    out.time.minute = dabtime_.Minutes;
    out.time.second = dabtime_.Seconds;

    cachedDateTime_ = out;
    hasCachedDateTime_ = true;

    return true;
}
