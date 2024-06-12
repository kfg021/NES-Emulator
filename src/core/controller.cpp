#include "controller.hpp"

Controller::Controller(){
    buttons = 0;
}

uint8_t Controller::getButtons(){
    return buttons;
}

void Controller::setButton(Button button, bool value){
    buttons &= ~(1 << button);
    if(value){
        buttons |= (1 << button);
    }
}