#include "bus.hpp"

 Bus::Bus(const std::shared_ptr<Cartridge>& cartridge) :
    cartridge(cartridge){
    ram = {};
}

uint8_t Bus::read(uint16_t address) const{
    if(RAM_ADDRESSABLE_RANGE.contains(address)){
        return ram[address & 0x7FF];
    }
    else if(PPU_ADDRESSABLE_RANGE.contains(address)){
        return 0; // TODO: implement PPU
    }
    else if(APU_ADDRESSABLE_RANGE.contains(address)){
        return 0; // TODO: implement APU
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        std::optional<uint8_t> data = cartridge->readFromPRG(address);
        if(data.has_value()){
            return data.value();
        }
        else{
            return 0; // TODO: what happens when read fails?
        }
    }
}

void Bus::write(uint16_t address, uint8_t value){
    if(RAM_ADDRESSABLE_RANGE.contains(address)){
        ram[address & 0x7FF] = value;
    }
    else if(PPU_ADDRESSABLE_RANGE.contains(address)){
        // TODO: implement PPU
    }
    else if(APU_ADDRESSABLE_RANGE.contains(address)){
        // TODO: implement APU
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        cartridge->writeToPRG(address, value);
        // TODO: what happens when write fails?
    }
}