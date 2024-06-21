#include "bus.hpp"

 Bus::Bus(){
    cpu = std::make_unique<CPU>();
    cpu->setBus(this);
    
    ppu = std::make_unique<PPU>();
    ppu->setBus(this);
 }

void Bus::initDevices(){
    ram = {};

    controllers = {};
    controllerData = {};

    cpu->initCPU();
    ppu->initPPU();

    nmiRequest = false;
    totalCycles = 0;

    dmaTransferRequested = false;
    dmaTransferOngoing = false;
    dmaPage = 0;
    dmaOffset = 0;
    dmaData = 0;
}

Cartridge::Status Bus::loadROM(const std::string& filePath){
    cartridge = std::make_shared<Cartridge>(filePath);

    Cartridge::Status status = cartridge->getStatus();
    if(status == Cartridge::Status::SUCCESS){
        ppu->setCartridge(cartridge);
        initDevices();
    }
    
    return status;
}

uint8_t Bus::view(uint16_t address) const{
    if(RAM_ADDRESSABLE_RANGE.contains(address)){
        return ram[address & 0x7FF];
    }
    else if(PPU_ADDRESSABLE_RANGE.contains(address)){
        return ppu->view(address & 0x7);
    }
    else if(IO_ADDRESSABLE_RANGE.contains(address)){
        if(address == CONTROLLER_1_DATA || address == CONTROLLER_2_DATA){
            // The view mode returns all controller outputs at once
            return controllerData[address & 1];
        }
        else{
            // TODO: Implement APU
            return 0;
        } 
        
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        std::optional<uint8_t> data = cartridge->viewPRG(address);
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
    else if(IO_ADDRESSABLE_RANGE.contains(address)){
        if(address == CONTROLLER_1_DATA || address == CONTROLLER_2_DATA){
            uint8_t data = controllerData[address & 1] & 1;
            controllerData[address & 1] >>= 1;
            return data;
        }
        else{
            // TODO: Implement APU
            return 0;
        } 
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
    else if(IO_ADDRESSABLE_RANGE.contains(address)){
        // TODO: Implement strobe
        if(address == CONTROLLER_1_DATA){
            controllerData[0] = controllers[0].getButtons();
            controllerData[1] = controllers[1].getButtons();
        }
        else if(address == OAM_DMA_ADDR){
            dmaTransferRequested = true;
            dmaDummyCycleNeeded = true;
            dmaPage = value;
        }
        else{
            // TODO: Implement APU
        }
    }
    else{ // if(CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        cartridge->writeToPRG(address, value); // TODO: what happens when write fails?
    }
}

void Bus::executeCycle(){
    ppu->executeCycle();

    if(totalCycles % 3 == 0){
        // TODO: Correctly implement dummy cycle
        if(dmaTransferRequested){
            doDmaTransferCycle();
        }
        else{
            cpu->executeCycle();
        }
    }

    if(nmiRequest){
        cpu->NMI();
        nmiRequest = false;
    }

    totalCycles++;
}

void Bus::doDmaTransferCycle(){
    if(dmaDummyCycleNeeded){
        dmaDummyCycleNeeded = false;
    }
    else{
        bool cycleMod = totalCycles % 2;
    
        if(!dmaTransferOngoing && cycleMod == 0){
            dmaTransferOngoing = true;
        }

        if(dmaTransferOngoing){
            if(cycleMod == 0){
                uint16_t dmaAddress = (dmaPage << 8) | dmaOffset;
                dmaData = read(dmaAddress);
            }
            else{
                ppu->oamBuffer[dmaOffset] = dmaData;
                dmaOffset++;
                if(dmaOffset == 0){
                    dmaTransferRequested = false;
                    dmaTransferOngoing = false;
                }
            }
        }
    }
}

void Bus::setController(bool controller, Controller::Button button, bool value){
    controllers[controller].setButton(button, value);
}