#include "UiController.hpp"

#include <string.h>

#include <cstring>

#include "../time/core/TimeMath.hpp"

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

/**
 * @internal
 * @brief Invalidate cached LCD line content.
 * @details
 * Forces the next render pass to refresh both LCD rows regardless of previously cached values.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::invalidateLcdCache() {
    lcdCacheValid_ = false;
}

/**
 * @internal
 * @brief Write one LCD row only when content changed.
 * @details
 * Normalizes text to 8 characters, compares against cached row content, and writes to the LCD
 * only on actual content changes to reduce I2C traffic.
 *
 * @param row LCD row index (0 or 1).
 * @param text Raw text to render.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::writeLcdIfChanged(uint8_t row, const String& text) {
    String normalized = text;
    if (normalized.length() < 8) {
        while (normalized.length() < 8) normalized += ' ';
    } else if (normalized.length() > 8) {
        normalized = normalized.substring(0, 8);
    }

    String* lastLine = (row == 0) ? &lastLcdLine0_ : &lastLcdLine1_;
    if (!lcdCacheValid_ || *lastLine != normalized) {
        lcd_.setCursor(0, row);
        lcd_.print(normalized);
        *lastLine = normalized;
        lcdCacheValid_ = true;
    }
}

// =========================================================================
// Input reading
// =========================================================================

// TODO: Fix the 3s SET long-press detection (currently instant changes to the source selection mode before waiting for a potential 3s press that should trigger manual config mode)
/**
 * @internal
 * @brief Poll and decode all UI inputs.
 * @details
 * Updates SET/CFG button edge and long-press states, synchronizes MCP23017 inputs,
 * and computes encoder movement delta from Gray-code transitions.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
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
    constexpr unsigned long ENCODER_DEBOUNCE_MS = 20UL;
    static unsigned long lastEncTimestamp = 0;

    uint8_t a = ioDeviceDigitalRead(mcpIo_, MCP_ENC_A);
    uint8_t b = ioDeviceDigitalRead(mcpIo_, MCP_ENC_B);
    uint8_t encState = (a << 1) | b;
    encDelta_ = 0;

    unsigned long now = millis();
    if ((lastEncTimestamp == 0UL || now - lastEncTimestamp >= ENCODER_DEBOUNCE_MS) &&
        encState != encLastState_) {
        lastEncTimestamp = now;
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

/**
 * @internal
 * @brief Report CFG pressed edge state.
 * @details
 * Returns whether a CFG rising edge was detected during the current cycle.
 *
 * @return True if CFG has just been pressed; false otherwise.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool UiController::cfgPressed() const { return cfgPressed_; }

/**
 * @internal
 * @brief Check whether CFG long-press threshold is reached.
 * @details
 * Evaluates held state and press duration against `LONG_PRESS_MS`.
 *
 * @return True when CFG is currently held long enough; false otherwise.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool UiController::cfgLongPressed() const {
    return cfgHeld_ && (millis() - cfgPressStartMs_ >= LONG_PRESS_MS);
}

/**
 * @internal
 * @brief Report SET pressed edge state.
 * @details
 * Returns whether a SET rising edge is currently active.
 *
 * @return True if SET has just been pressed; false otherwise.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool UiController::setPressed() const {
    return setButton_.isPressed();
}

/**
 * @internal
 * @brief Report SET long-press state.
 * @details
 * Returns the internal long-press flag computed from button hold duration.
 *
 * @return True when SET long press is considered active; false otherwise.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool UiController::setLongPressed() const {
    return setHeld_;
}

/**
 * @internal
 * @brief Get encoder delta for current cycle.
 * @details
 * Returns signed movement computed from the latest quadrature transition.
 *
 * @return Encoder movement delta (-1, 0, or +1 for half-step decoding).
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
int8_t UiController::encoderDelta() const { return encDelta_; }

/**
 * @internal
 * @brief Report CFG release edge state.
 * @details
 * Returns whether a CFG falling edge was detected during the current cycle.
 *
 * @return True if CFG has just been released; false otherwise.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
bool UiController::cfgReleased() const { return cfgReleased_; }

// =========================================================================
// Mode: DisplayDateTime
// =========================================================================

/**
 * @internal
 * @brief Handle UI logic in normal date/time display mode.
 * @details
 * Processes transitions to error, source selection, and manual configuration modes,
 * triggers manual sync on SET press, and renders default clock view.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
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
            ::memset(&manualDt_, 0, sizeof(manualDt_));
            manualDt_.date.year = 2026;
            manualDt_.date.month = 1;
            manualDt_.date.day = 1;
        }
        invalidateLcdCache();
        return;
    }

    if (cfgReleased()) {
        // Enter source selection mode
        if (logger_) logger_->debug("Entering source selection mode");
        mode_ = Mode::SourceSelection;
        selectedSourceIdx_ = 0;
        invalidateLcdCache();
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

/**
 * @internal
 * @brief Handle UI logic while an error is active.
 * @details
 * Displays the highest-priority active error and acknowledges all errors when SET is pressed.
 * Automatically returns to display mode once no active error remains.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::handleErrorPresent() {
    // If no more active errors, return to display
    if (!coordinator_.errors().hasActiveError()) {
        mode_ = Mode::DisplayDateTime;
        invalidateLcdCache();
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

/**
 * @internal
 * @brief Handle UI logic in source selection mode.
 * @details
 * Supports source navigation with encoder, enable/disable toggling with SET, and mode exit on CFG release.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::handleSourceSelection() {
    // CFG pressed again → exit source selection
    if (cfgReleased()) {
        mode_ = Mode::DisplayDateTime;
        invalidateLcdCache();
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

/**
 * @internal
 * @brief Handle UI logic in manual date/time configuration mode.
 * @details
 * Applies field navigation and editing, supports cancel on SET long press, and saves
 * the configured date-time on CFG release after debounce.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::handleManualConfig() {
    // Long press SET (3 s) → exit without saving
    if (setLongPressed()) {
        mode_ = Mode::DisplayDateTime;
        invalidateLcdCache();
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
        invalidateLcdCache();
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

/**
 * @internal
 * @brief Render default date/time screen.
 * @details
 * Throttles redraw frequency, shows current coordinator time when available,
 * or displays a fallback message when no valid time is present.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
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

/**
 * @internal
 * @brief Render current synchronization error on LCD.
 * @details
 * Draws a compact two-line error view and pads label text to fit the 8-character display width.
 *
 * @param err Pointer to the error entry to display.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::renderError(const SyncError* err) {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    if (!err) return;
    writeLcdIfChanged(0, "Err:");
    // Print label, pad to 8 chars
    String lbl = err->label();
    writeLcdIfChanged(1, lbl);
}

/**
 * @internal
 * @brief Render source selection screen.
 * @details
 * Displays selected source name and enabled/disabled state with fixed-width padding.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::renderSourceSelection() {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    writeLcdIfChanged(0, "Src sel:");

    if (selectedSourceIdx_ < sourceCount_ && sources_[selectedSourceIdx_]) {
        bool on = sources_[selectedSourceIdx_]->isEnabled();
        String line = String(sourceName(selectedSourceIdx_));
        line += on ? " ON " : " OFF";
        writeLcdIfChanged(1, line);
    } else {
        writeLcdIfChanged(1, "");
    }
}

/**
 * @internal
 * @brief Render manual configuration screen.
 * @details
 * Displays current editable field on the first line and the corresponding value on the second line.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
void UiController::renderManualConfig() {
    if (millis() - lastRenderMs_ < RENDER_INTERVAL_MS) return;
    lastRenderMs_ = millis();

    // Line 1: field name
    const char* fieldNames[] = {"Hour", "Minute", "Second", "DOW", "Day", "Month", "Year"};
    String header = fieldNames[manualFieldIdx_];
    writeLcdIfChanged(0, header);

    // Line 2: current value (with blinking cursor effect via modular time)
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
    writeLcdIfChanged(1, val);
}

// =========================================================================
// Helpers
// =========================================================================

/**
 * @internal
 * @brief Resolve source index to display name.
 * @details
 * Converts internal source index values into short labels used in the UI.
 *
 * @param index Source index value.
 * @return Constant string label associated with the index.
 *
 * @author GOLETTA David
 * @date 02/03/2026
 * @endinternal
 */
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
