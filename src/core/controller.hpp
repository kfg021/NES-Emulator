#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <cstdint>

class Controller {
public:
    Controller();
    enum Button {
        A,
        B,
        SELECT,
        START,
        UP,
        DOWN,
        LEFT,
        RIGHT,
    };

    uint8_t getButtons();
    void setButton(Button button, bool value);

private:
    uint8_t buttons;
};

#endif // CONTROLLER_HPP