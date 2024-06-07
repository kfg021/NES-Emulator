#include "ppu.hpp"

PPU::PPU(){
}

void PPU::setBus(Bus* bus){
    this->bus = bus;
}

void PPU::setCartridge(std::shared_ptr<Cartridge>& cartridge){
    this->cartridge = cartridge;
}

std::optional<uint8_t> PPU::read(uint16_t address){
    if(address == STATUS){
        return std::nullopt;
    }
    else if(address == OAMDATA){
        return std::nullopt;
    }
    else if(address == DATA){
        return std::nullopt;
    }
    else{
        return std::nullopt;
    }
}

void PPU::write(uint16_t address, uint8_t /*value*/){
    if(address == CONTROL){
    }
    else if(address == MASK){

    }
    else if(address == OAMADDR){

    }
    else if(address == OAMDATA){

    }
    else if(address == SCROLL){

    }
}