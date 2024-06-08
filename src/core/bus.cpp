#include "bus.hpp"

 Bus::Bus(){
    cpu = std::make_unique<CPU>();
    cpu->setBus(this);
    
    ppu = std::make_unique<PPU>();
    ppu->setBus(this);
 }

void Bus::initDevices(){
    ram = {};
    cpu->initCPU();
    ppu->initPPU();

    nmiRequest = false;
    totalCycles = 0;
}

bool Bus::loadROM(const std::string& filePath){
    cartridge = std::make_shared<Cartridge>(filePath);
    if(!cartridge->isValid()){
        return false;
    }
    ppu->setCartridge(cartridge);

    initDevices();
    return true;
}

uint8_t Bus::view(uint16_t address) const{
    if(RAM_ADDRESSABLE_RANGE.contains(address)){
        return ram[address & 0x7FF];
    }
    else if(PPU_ADDRESSABLE_RANGE.contains(address)){
        return ppu->view(address & 0x7);
    }
    else if(APU_ADDRESSABLE_RANGE.contains(address)){
        return 0; // TODO: implement APU
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        std::optional<uint8_t> data = cartridge->readFromPRG(address);
        return data.value_or(0);
    }
}

uint8_t Bus::read(uint16_t address){
    if(RAM_ADDRESSABLE_RANGE.contains(address)){
        return ram[address & 0x7FF];
    }
    else if(PPU_ADDRESSABLE_RANGE.contains(address)){
        return ppu->read(address & 0x7);
    }
    else if(APU_ADDRESSABLE_RANGE.contains(address)){
        return 0; // TODO: implement APU
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        std::optional<uint8_t> data = cartridge->readFromPRG(address);
        return data.value_or(0);
    }
}

void Bus::write(uint16_t address, uint8_t value){
    if(RAM_ADDRESSABLE_RANGE.contains(address)){
        ram[address & 0x7FF] = value;
    }
    else if(PPU_ADDRESSABLE_RANGE.contains(address)){
        ppu->write(address & 0x7, value); // TODO: what happens when write fails?
    }
    else if(APU_ADDRESSABLE_RANGE.contains(address)){
        // TODO: implement APU
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        cartridge->writeToPRG(address, value); // TODO: what happens when write fails?
    }
}

void Bus::executeCycle(){
    ppu->executeCycle();

    if(totalCycles % 3 == 0){
        cpu->executeCycle();
    }

    if(nmiRequest){
        cpu->NMI();
        nmiRequest = false;
    }

    totalCycles++;
}