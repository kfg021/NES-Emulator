#include "controller.hpp"

Controller::Controller() {
    buttons = 0;
}

uint8_t Controller::getButtons() {
    uint8_t newButtons = buttons;

    // NES controllers could not handle the (U and D) or (L and R) buttons being pressed simultaneously
    removeOppositeInputs(newButtons, UP, DOWN);
    removeOppositeInputs(newButtons, LEFT, RIGHT);

    return newButtons;
}

void Controller::setButton(Button button, bool value) {
    buttons &= ~(1 << button);
    if (value) {
        buttons |= (1 << button);
    }
}

void Controller::removeOppositeInputs(uint8_t& originalButtons, Button a, Button b) const {
    bool aPressed = (originalButtons >> a) & 1;
    bool bPressed = (originalButtons >> b) & 1;
    if (aPressed && bPressed) {
        originalButtons &= ~(1 << a);
        originalButtons &= ~(1 << b);
    }
}