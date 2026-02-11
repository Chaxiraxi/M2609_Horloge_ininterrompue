#ifndef BUTTON_H
#define BUTTON_H

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
     *
     * @note This method must be called exactly once per loop to ensure correct state tracking.
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    void updateState();

    /**
     * @brief Get the current state of the button
     *
     * @return State The current state of the button
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    State getState();

    /**
     * @brief Check if the button is currently pressed
     *
     * @return true if the button is pressed (rising edge), false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isPressed();

    /**
     * @brief Check if the button is currently released
     *
     * @return true if the button is released (falling edge), false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isReleased();

    /**
     * @brief Check if the button is currently held
     *
     * @return true if the button is held, false otherwise
     *
     * @author GOLETTA David
     * @date 27/11/2025
     */
    bool isHeld();

    /**
     * @brief Check if the button is currently idle
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

#endif  // BUTTON_H