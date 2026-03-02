#pragma once

/**
 * @file Button.h
 * @brief Declaration of the Button class for handling button states.
 *
 * @details
 * This file contains the declaration of the Button class, which provides
 * functionality to read and interpret the state of a button connected to
 * a specified pin. The button can be in one of four states: PRESSED, HOLD,
 * RELEASED, or IDLE. This is useful for easy edge detection and state management.
 *
 * @author GOLETTA David
 * @date 27/11/2025
 */
class Button {
   public:
    /**
     * @brief Construct a new Button object
     * @details
     * Description:
     *   Stores the GPIO pin number and initializes internal button state tracking.
     *
     * @param pin The pin number to which the button is connected
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    Button(int pin);

    /**
     * @brief Possible states of the button
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    enum State {
        PRESSED,
        HOLD,
        RELEASED,
        IDLE
    };

    /**
     * @brief Update the current state of the button
     * @details
     * Description:
     *   Samples the input pin and updates the internal state machine for edge/hold detection.
     *
     * @note This method must be called exactly once per loop to ensure correct state tracking.
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    void updateState();

    /**
     * @brief Get the current state of the button
     * @details
     * Description:
     *   Returns the current state computed by the latest call to updateState().
     *
     * @return State The current state of the button
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    State getState();

    /**
     * @brief Check if the button is currently pressed
     * @details
     * Description:
     *   Reports whether the current state corresponds to a press edge.
     *
     * @return true if the button is pressed (rising edge), false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isPressed();

    /**
     * @brief Check if the button is currently released
     * @details
     * Description:
     *   Reports whether the current state corresponds to a release edge.
     *
     * @return true if the button is released (falling edge), false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isReleased();

    /**
     * @brief Check if the button is currently held
     * @details
     * Description:
     *   Reports whether the button remains continuously pressed.
     *
     * @return true if the button is held, false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isHeld();

    /**
     * @brief Check if the button is currently idle
     * @details
     * Description:
     *   Reports whether the button is currently not pressed and no edge is active.
     *
     * @return true if the button is idle, false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isIdle();

   private:
    int _pin;
    State _lastState;
    State _currentState;
};
