#include "core/controller.hpp"

Controller::Controller() {
    buttons = 0;
}

uint8_t Controller::getButtons() {
    uint8_t newButtons = buttons;

    // NES controllers could not handle the (U and D) or (L and R) buttons being pressed simultaneously
    removeOppositeInputs(newButtons, Button::UP, Button::DOWN);
    removeOppositeInputs(newButtons, Button::LEFT, Button::RIGHT);

    return newButtons;
}

void Controller::setButton(Button button, bool value) {
    buttons &= ~(1 << static_cast<int>(button));
    if (value) {
        buttons |= (1 << static_cast<int>(button));
    }
}

void Controller::removeOppositeInputs(uint8_t& originalButtons, Button a, Button b) const {
    bool aPressed = (originalButtons >> static_cast<int>(a)) & 1;
    bool bPressed = (originalButtons >> static_cast<int>(b)) & 1;
    if (aPressed && bPressed) {
        originalButtons &= ~(1 << static_cast<int>(a));
        originalButtons &= ~(1 << static_cast<int>(b));
    }
}