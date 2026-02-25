#include "UiController.h"

#include <cstring>

#include "TimeMath.h"

// =========================================================================
// Construction / init
// =========================================================================

UiController::UiController(LiquidCrystal& lcd, Button& setButton,
                           TimeCoordinator& coordinator,
                           TimeSource* sources[], uint8_t sourceCount,
                           IoAbstractionRef mcpIo, Notification* logger)
    : lcd_(lcd), setButton_(setButton), coordinator_(coordinator), mcpIo_(mcpIo), logger_(logger) {
    sourceCount_ = min(sourceCount, TimeCoordinator::MAX_SOURCES);
    for (uint8_t i = 0; i < sourceCount_; ++i) {
        sources_[i] = sources[i];
    }
}

void UiController::begin() {
    // Configure MCP23017 pins as inputs
    ioDevicePinMode(mcpIo_, MCP_CFG_BTN, INPUT);
    ioDevicePinMode(mcpIo_, MCP_ENC_A, INPUT);
    ioDevicePinMode(mcpIo_, MCP_ENC_B, INPUT);
    ioDeviceSync(mcpIo_);

    // Initialise encoder last state
    encLastState_ = (ioDeviceDigitalRead(mcpIo_, MCP_ENC_A) << 1) |
                    ioDeviceDigitalRead(mcpIo_, MCP_ENC_B);
}

// =========================================================================
// Main update — called every loop()
// =========================================================================

void UiController::update() {
    readInputs();

    switch (mode_) {
        case Mode::DisplayDateTime:
            handleDisplayDateTime();
            break;
        case Mode::ErrorPresent:
            handleErrorPresent();
            break;
        case Mode::SourceSelection:
            handleSourceSelection();
            break;
        case Mode::ManualConfig:
            handleManualConfig();
            break;
    }
}

// =========================================================================
// Input reading
// =========================================================================

// TODO: Fix the 3s SET long-press detection (currently instant changes to the source selection mode before waiting for a potential 3s press that should trigger manual config mode)
void UiController::readInputs() {
    // --- SET button (GPIO, handled by Button class) ---
    setButton_.updateState();
    if (setButton_.isPressed()) {
        setPressStartMs_ = millis();
        setHeld_ = false;
    }
    if (setButton_.isHeld()) {
        if (!setHeld_ && (millis() - setPressStartMs_ >= LONG_PRESS_MS)) {
            setHeld_ = true;  // long-press detected
        }
    }
    if (setButton_.isReleased()) {
        setHeld_ = false;
    }

    // --- CFG button (MCP23017 pin, manual polling with debounce) ---
    ioDeviceSync(mcpIo_);
    bool cfgNow = (ioDeviceDigitalRead(mcpIo_, MCP_CFG_BTN) == HIGH);  // active-high
    cfgPressed_ = (cfgNow && !cfgLast_);                               // rising edge
    cfgReleased_ = (!cfgNow && cfgLast_);                              // falling edge
    if (cfgPressed_) {
        cfgPressStartMs_ = millis();
    }
    cfgHeld_ = cfgNow;
    cfgLast_ = cfgNow;

    // --- Encoder (Gray-code on MCP23017) ---
    uint8_t a = ioDeviceDigitalRead(mcpIo_, MCP_ENC_A);
    uint8_t b = ioDeviceDigitalRead(mcpIo_, MCP_ENC_B);
    uint8_t encState = (a << 1) | b;
    encDelta_ = 0;
    if (encState != encLastState_) {
        // Simple quadrature decode (half-step)
        // Direction table for Gray code transitions:
        //  00->01 = +1, 00->10 = -1
        //  01->11 = +1, 01->00 = -1
        //  11->10 = +1, 11->01 = -1
        //  10->00 = +1, 10->11 = -1
        static const int8_t encTable[16] = {
            0, +1, -1, 0,
            -1, 0, 0, +1,
            +1, 0, 0, -1,
            0, -1, +1, 0};
        encDelta_ = encTable[(encLastState_ << 2) | encState];
        encLastState_ = encState;
    }
}

bool UiController::cfgPressed() const { return cfgPressed_; }

bool UiController::cfgLongPressed() const {
    return cfgHeld_ && (millis() - cfgPressStartMs_ >= LONG_PRESS_MS);
}

bool UiController::setPressed() const {
    return setButton_.isPressed();
}

bool UiController::setLongPressed() const {
    return setHeld_;
}

int8_t UiController::encoderDelta() const { return encDelta_; }

bool UiController::cfgReleased() const { return cfgReleased_; }

// =========================================================================
// Mode: DisplayDateTime
// =========================================================================

void UiController::handleDisplayDateTime() {
    // Transitions
    if (coordinator_.errors().hasActiveError()) {
        mode_ = Mode::ErrorPresent;
        return;
    }

    if (cfgLongPressed()) {
        // Enter manual config mode
        mode_ = Mode::ManualConfig;
        manualFieldIdx_ = 0;
        // Pre-fill with current time (or zeros)
        if (!coordinator_.getCurrentDateTime(manualDt_)) {
            memset(&manualDt_, 0, sizeof(manualDt_));
            manualDt_.date.year = 2026;
            manualDt_.date.month = 1;
            manualDt_.date.day = 1;
        }
        lcd_.clear();
        return;
    }

    if (cfgReleased()) {
        // Enter source selection mode
        if (logger_) logger_->debug("Entering source selection mode");
        mode_ = Mode::SourceSelection;
        selectedSourceIdx_ = 0;
        lcd_.clear();
        return;
    }

    // SET in normal mode (no errors) → manual sync
    if (setButton_.isPressed()) {
        coordinator_.forceSync();
    }

    renderDateTime();
}

// =========================================================================
// Mode: ErrorPresent
// =========================================================================

void UiController::handleErrorPresent() {
    // If no more active errors, return to display
    if (!coordinator_.errors().hasActiveError()) {
        mode_ = Mode::DisplayDateTime;
        lcd_.clear();
        return;
    }

    // SET acknowledges all errors
    if (setButton_.isPressed()) {
        if (logger_) logger_->debug("Acknowledging all errors");
        coordinator_.errors().acknowledgeAll();
        // After ack, if all clear, next update() will return to DisplayDateTime.
        // SET does NOT trigger manual sync here (errors were pending).
        return;
    }

    const SyncError* err = coordinator_.errors().firstActive();
    renderError(err);
}

// =========================================================================
// Mode: SourceSelection
// =========================================================================

void UiController::handleSourceSelection() {
    // CFG pressed again → exit source selection
    if (cfgReleased()) {
        mode_ = Mode::DisplayDateTime;
        lcd_.clear();
        if (logger_) logger_->debug("Exiting source selection mode");
        return;
    }

    // Encoder scrolls through sources
    int8_t delta = encoderDelta();
    if (delta != 0) {
        int8_t next = static_cast<int8_t>(selectedSourceIdx_) + delta;
        if (next < 0) next = sourceCount_ - 1;
        if (next >= sourceCount_) next = 0;
        selectedSourceIdx_ = static_cast<uint8_t>(next);
    }

    // SET toggles the selected source
    if (setButton_.isPressed() && selectedSourceIdx_ < sourceCount_ && sources_[selectedSourceIdx_]) {
        bool enabled = sources_[selectedSourceIdx_]->isEnabled();
        sources_[selectedSourceIdx_]->setEnabled(!enabled);
        if (logger_) logger_->debug("Toggled source " + String(selectedSourceIdx_) + " to " + (enabled ? "disabled" : "enabled"));
    }

    renderSourceSelection();
}

// =========================================================================
// Mode: ManualConfig
// =========================================================================

void UiController::handleManualConfig() {
    // Long press SET (3 s) → exit without saving
    if (setLongPressed()) {
        mode_ = Mode::DisplayDateTime;
        lcd_.clear();
        return;
    }

    // CFG pressed → save and exit
    static bool exit_debounced = false;
    if (cfgReleased()) {
        if (!exit_debounced) {
            exit_debounced = true;  // ignore the first release (from entering the mode)
            return;
        }
        coordinator_.setManualDateTime(manualDt_);
        mode_ = Mode::DisplayDateTime;
        lcd_.clear();
        exit_debounced = false;  // reset debounce for next entry
        return;
    }

    // SET pressed → next field
    if (setButton_.isPressed()) {
        manualFieldIdx_ = (manualFieldIdx_ + 1) % FIELD_COUNT;
        // Skip day-of-week (it's derived, not editable)
        if (manualFieldIdx_ == FIELD_DOW) {
            manualFieldIdx_ = (manualFieldIdx_ + 1) % FIELD_COUNT;
        }
    }

    // Encoder changes the current field value
    int8_t delta = encoderDelta();
    if (delta != 0) {
        switch (static_cast<ManualField>(manualFieldIdx_)) {
            case FIELD_HOUR:
                manualDt_.time.hour = (manualDt_.time.hour + delta + 24) % 24;
                break;
            case FIELD_MINUTE:
                manualDt_.time.minute = (manualDt_.time.minute + delta + 60) % 60;
                break;
            case FIELD_SECOND:
                manualDt_.time.second = (manualDt_.time.second + delta + 60) % 60;
                break;
            case FIELD_DAY: {
                uint8_t maxD = TimeMath::daysInMonth(manualDt_.date.year, manualDt_.date.month);
                int8_t d = static_cast<int8_t>(manualDt_.date.day) + delta;
                if (d < 1) d = maxD;
                if (d > maxD) d = 1;
                manualDt_.date.day = static_cast<uint8_t>(d);
                break;
            }
            case FIELD_MONTH: {
                int8_t m = static_cast<int8_t>(manualDt_.date.month) + delta;
                if (m < 1) m = 12;
                if (m > 12) m = 1;
                manualDt_.date.month = static_cast<uint8_t>(m);
                break;
            }
            case FIELD_YEAR:
                manualDt_.date.year = static_cast<uint16_t>(
                    max(1970, static_cast<int>(manualDt_.date.year) + delta));
                break;
            default:
                break;
        }
    }

    renderManualConfig();
}

// =========================================================================
// Rendering
// =========================================================================

void UiController::renderDateTime() {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    DateTimeFields dt{};
    if (coordinator_.getCurrentDateTime(dt)) {
        printTimeDateOnScreen(&lcd_, dt.time.hour, dt.time.minute, dt.time.second,
                              dt.date.day, dt.date.month, dt.date.year);
    } else {
        displayLongText(&lcd_, "No time source found");
    }
}

void UiController::renderError(const SyncError* err) {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    if (!err) return;
    lcd_.setCursor(0, 0);
    lcd_.print("Err:    ");
    lcd_.setCursor(0, 1);
    // Print label, pad to 8 chars
    String lbl = err->label();
    while (lbl.length() < 8) lbl += ' ';
    lcd_.print(lbl.substring(0, 8));
}

void UiController::renderSourceSelection() {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    lcd_.setCursor(0, 0);
    lcd_.print("Src sel:");

    lcd_.setCursor(0, 1);
    if (selectedSourceIdx_ < sourceCount_ && sources_[selectedSourceIdx_]) {
        bool on = sources_[selectedSourceIdx_]->isEnabled();
        String line = String(sourceName(selectedSourceIdx_));
        line += on ? " ON " : " OFF";
        while (line.length() < 8) line += ' ';
        lcd_.print(line.substring(0, 8));
    } else {
        lcd_.print("        ");
    }
}

void UiController::renderManualConfig() {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    // Line 1: field name
    lcd_.setCursor(0, 0);
    const char* fieldNames[] = {"Hour", "Minute", "Second", "DOW", "Day", "Month", "Year"};
    String header = fieldNames[manualFieldIdx_];
    while (header.length() < 8) header += ' ';
    lcd_.print(header.substring(0, 8));

    // Line 2: current value (with blinking cursor effect via modular time)
    lcd_.setCursor(0, 1);
    String val;
    switch (static_cast<ManualField>(manualFieldIdx_)) {
        case FIELD_HOUR:
            val = formatTwoDigits(manualDt_.time.hour);
            break;
        case FIELD_MINUTE:
            val = formatTwoDigits(manualDt_.time.minute);
            break;
        case FIELD_SECOND:
            val = formatTwoDigits(manualDt_.time.second);
            break;
        case FIELD_DAY:
            val = formatTwoDigits(manualDt_.date.day);
            break;
        case FIELD_MONTH:
            val = formatTwoDigits(manualDt_.date.month);
            break;
        case FIELD_YEAR:
            val = String(manualDt_.date.year);
            break;
        default:
            val = "--";
            break;
    }
    while (val.length() < 8) val += ' ';
    lcd_.print(val.substring(0, 8));
}

// =========================================================================
// Helpers
// =========================================================================

const char* UiController::sourceName(uint8_t index) const {
    switch (index) {
        case 0:
            return "DAB";
        case 1:
            return "NTP";
        case 2:
            return "GPS";
        default:
            return "???";
    }
}
