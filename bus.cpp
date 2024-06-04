#include "bus.hpp"

 Bus::Bus(){
    clear();
 }

void Bus::clear(){
    data = {};
}

uint8_t Bus::read(uint16_t address) const{
    return data[address];
}

void Bus::write(uint16_t address, uint8_t value){
    data[address] = value;
}