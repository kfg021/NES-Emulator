#include "ppu.hpp"

PPU::PPU(){
}

void PPU::setBus(const std::shared_ptr<Bus>& bus){
    this->bus = bus;
}