#include "bus.hpp"

 Bus::Bus(){
    cpu = std::make_unique<CPU>();
    cpu->setBus(this);
    
    ppu = std::make_unique<PPU>();
    ppu->setBus(this);
 }

void Bus::reset(){
    cpu->initCPU();
}

bool Bus::loadROM(const std::string& filePath){
    cartridge = std::make_unique<Cartridge>(filePath);
    if(!cartridge->isValid()){
        return false;
    }

    reset();
    return true;
}

uint8_t Bus::read(uint16_t address){
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