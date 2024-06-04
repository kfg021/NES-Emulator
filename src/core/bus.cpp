#include "bus.hpp"

 Bus::Bus(){
    ram = {};
}

uint8_t Bus::read(uint16_t address) const{
    // if(RAM_RANGE.contains(address)){
    //     return ram[address & 0x07FF];
    // }
    return ram[address];
}

void Bus::write(uint16_t address, uint8_t value){
    // if(RAM_RANGE.contains(address)){
    //     ram[address & 0x07FF] = value;
    // }
    ram[address] = value;
}