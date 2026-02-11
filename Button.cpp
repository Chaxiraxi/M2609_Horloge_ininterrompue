#include "Button.h"

#include <Arduino.h>

Button::Button(int pin) : _pin(pin), _lastState(IDLE), _currentState(IDLE) {}

void Button::updateState() {
    int buttonState = digitalRead(_pin);
    State currentState = IDLE;

    if (buttonState == HIGH) {
        if (_lastState == IDLE || _lastState == RELEASED) {
            currentState = PRESSED;
        } else {
            currentState = HOLD;
        }
    } else {
        if (_lastState == PRESSED || _lastState == HOLD) {
            currentState = RELEASED;
        } else {
            currentState = IDLE;
        }
    }

    _lastState = currentState;
    _currentState = currentState;
}

Button::State Button::getState() { return _currentState; }

bool Button::isPressed() { return _currentState == PRESSED; }

bool Button::isReleased() { return _currentState == RELEASED; }

bool Button::isHeld() { return _currentState == HOLD; }

bool Button::isIdle() { return _currentState == IDLE; }