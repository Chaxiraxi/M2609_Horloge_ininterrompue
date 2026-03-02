#pragma once

#include <IoAbstractionWire.h>
#include <LiquidCrystalIO.h>

#include "Button.h"
#include "Notification.h"
#include "PinDefinitions.h"
#include "SyncErrors.h"
#include "TimeCoordinator.h"
#include "TimeSource.h"
#include "utils.h"

/**
 * @file UiController.h
 * @brief State machine that drives the LCD + button/encoder UI.
 *
 * @details
 * Description:
 *   Implements the four UI states described in the specification:
 *     - DisplayDateTime  : normal time display
 *     - ErrorPresent     : an error is shown, SET acknowledges
 *     - SourceSelection  : CFG enters/exits; ENC scrolls sources; SET toggles
 *     - ManualConfig     : long CFG enters; ENC changes digit; SET next field;
 *                          CFG saves+exits; long SET exits w/o saving
 *
 *   The controller reads SET from a GPIO Button, and CFG + encoder from the
 *   MCP23017 I/O expander (pins defined in PinDefinitions.h).
 *
 * @author GOLETTA David
 * @date 13/02/2026
 */
class UiController {
   public:
    enum class Mode : uint8_t {
        DisplayDateTime,
        ErrorPresent,
        SourceSelection,
        ManualConfig,
    };

    /**
     * @brief Construct the UI controller.
     * @details
     * Description:
     *   Stores hardware references and initializes controller state for LCD rendering,
     *   source selection, and manual time configuration.
     *
     * @param lcd          Reference to the 8×2 LiquidCrystal display.
     * @param setButton    Reference to the SET GPIO button.
     * @param coordinator  Reference to the central time coordinator.
     * @param sources      Array of source pointers (same order as coordinator).
     * @param sourceCount  Number of sources.
     * @param mcpIo        IoAbstractionRef for the MCP23017 at address 0x20.
     * @param logger       Optional logger for debug/warning messages.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    UiController(LiquidCrystal& lcd, Button& setButton,
                 TimeCoordinator& coordinator,
                 TimeSource* sources[], uint8_t sourceCount,
                 IoAbstractionRef mcpIo, Notification* logger = nullptr);

    /**
     * @brief One-time setup (pin modes for MCP inputs).
     * @details
     * Description:
     *   Configures input modes and initial states for UI controls connected to the MCP23017.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void begin();

    /**
     * @brief Call every loop iteration to process inputs and refresh the display.
     * @details
     * Description:
     *   Polls buttons/encoder, applies state-machine transitions, and redraws the LCD as needed.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    void update();

    /**
     * @brief Current UI mode (for external inspection / debugging).
     * @details
     * Description:
     *   Returns the current state-machine mode used by the controller.
     *
     * @return Current UiController::Mode value.
     *
     * @author GOLETTA David
     * @date 02/03/2026
     */
    Mode currentMode() const { return mode_; }

   private:
    // --- Input helpers ---------------------------------------------------
    void readInputs();
    bool cfgPressed() const;
    bool cfgLongPressed() const;
    bool setPressed() const;
    bool setLongPressed() const;
    bool cfgReleased() const;
    int8_t encoderDelta() const;

    // --- Mode handlers ---------------------------------------------------
    void handleDisplayDateTime();
    void handleErrorPresent();
    void handleSourceSelection();
    void handleManualConfig();

    // --- Display helpers -------------------------------------------------
    void renderDateTime();
    void renderError(const SyncError* err);
    void renderSourceSelection();
    void renderManualConfig();

    // --- Source name helper -----------------------------------------------
    const char* sourceName(uint8_t index) const;

    // --- References ------------------------------------------------------
    LiquidCrystal& lcd_;
    Button& setButton_;
    TimeCoordinator& coordinator_;
    TimeSource* sources_[TimeCoordinator::MAX_SOURCES] = {};
    uint8_t sourceCount_ = 0;
    IoAbstractionRef mcpIo_;
    Notification* logger_ = nullptr;

    Mode mode_ = Mode::DisplayDateTime;

    // CFG button state (on MCP, manual polling with debounce)
    bool cfgRaw_ = false;
    bool cfgLast_ = false;
    bool cfgPressed_ = false;   ///< rising edge this cycle
    bool cfgReleased_ = false;  ///< falling edge this cycle
    bool cfgHeld_ = false;      ///< currently held
    uint32_t cfgPressStartMs_ = 0;
    static constexpr uint32_t LONG_PRESS_MS = 3000;

    // SET long-press tracking (Button class gives us edges; we track duration)
    uint32_t setPressStartMs_ = 0;
    bool setHeld_ = false;

    // Encoder state (Gray-code polling on MCP)
    uint8_t encLastState_ = 0;
    int8_t encDelta_ = 0;

    // Source selection mode state
    uint8_t selectedSourceIdx_ = 0;

    // Manual config mode state
    enum ManualField : uint8_t {
        FIELD_HOUR = 0,
        FIELD_MINUTE,
        FIELD_SECOND,
        FIELD_DOW,  // day-of-week (display only, derived)
        FIELD_DAY,
        FIELD_MONTH,
        FIELD_YEAR,
        FIELD_COUNT
    };
    DateTimeFields manualDt_{};
    uint8_t manualFieldIdx_ = 0;

    // LCD refresh throttle
    uint32_t lastRenderMs_ = 0;
    static constexpr uint16_t RENDER_INTERVAL_MS = 100;
};
