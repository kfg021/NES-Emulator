#include "core/bus.hpp"

Bus::Bus() {
    cpu = std::make_unique<CPU>();
    cpu->setBus(this);

    ppu = std::make_unique<PPU>();
    ppu->setBus(this);

    apu = std::make_unique<APU>();
}

void Bus::initDevices() {
    ram = {};

    controllers = {};
    controllerData = {};
    strobe = 0;

    cpu->initCPU();
    ppu->initPPU();

    nmiRequest = false;
    irqRequest = false;

    totalCycles = 0;

    dmaTransferRequested = false;
    dmaTransferOngoing = false;
    dmaPage = 0;
    dmaOffset = 0;
    dmaData = 0;
}

Cartridge::Status Bus::loadROM(const std::string& filePath) {
    cartridge = std::make_shared<Cartridge>(filePath);

    Cartridge::Status status = cartridge->getStatus();
    if (status.code == Cartridge::Code::SUCCESS) {
        ppu->setCartridge(cartridge);
        initDevices();
    }

    return status;
}

uint8_t Bus::view(uint16_t address) const {
    if (RAM_ADDRESSABLE_RANGE.contains(address)) {
        return ram[address & 0x7FF];
    }
    else if (PPU_ADDRESSABLE_RANGE.contains(address)) {
        return ppu->view(address & 0x7);
    }
    else if (IO_ADDRESSABLE_RANGE.contains(address)) {
        if (address == CONTROLLER_1_DATA || address == CONTROLLER_2_DATA) {
            // The view mode returns all controller outputs at once
            return controllerData[address & 1];
        }
        else if (address == APU_STATUS) {
            return apu->viewStatus();
        }
        else {
            return 0;
        }
    }
    else { // if (CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        return cartridge->mapper->mapPRGView(address);
    }
}

uint8_t Bus::read(uint16_t address) {
    if (RAM_ADDRESSABLE_RANGE.contains(address)) {
        return ram[address & 0x7FF];
    }
    else if (PPU_ADDRESSABLE_RANGE.contains(address)) {
        return ppu->read(address & 0x7);
    }
    else if (IO_ADDRESSABLE_RANGE.contains(address)) {
        if (address == CONTROLLER_1_DATA || address == CONTROLLER_2_DATA) {
            uint8_t data = controllerData[address & 1] & 1;
            if (!strobe) {
                controllerData[address & 1] >>= 1;
            }
            return data;
        }
        else if (address == APU_STATUS) {
            return apu->readStatus();
        }
        else {
            return 0;
        }
    }
    else { // if (CARTRIDGE_ADDRESSABLE_RANGE.contains(address))
        return cartridge->mapper->mapPRGRead(address);
    }
}

void Bus::write(uint16_t address, uint8_t value) {
    if (RAM_ADDRESSABLE_RANGE.contains(address)) {
        ram[address & 0x7FF] = value;
    }
    else if (PPU_ADDRESSABLE_RANGE.contains(address)) {
        ppu->write(address & 0x7, value); // TODO: what happens when write fails?
    }
    else if (IO_ADDRESSABLE_RANGE.contains(address)) {
        if (APU_ADDRESSABLE_RANGE.contains(address)) {
            apu->write(address, value);
        }
        else if (address == CONTROLLER_1_DATA) {
            strobe = value & 1;
            if (strobe) {
                controllerData[0] = controllers[0].getButtons();
                controllerData[1] = controllers[1].getButtons();
            }
        }
        else if (address == OAM_DMA_ADDR) {
            dmaTransferRequested = true;
            dmaPage = value;
        }
        else if (address == APU_STATUS) {
            apu->writeStatus(value);
        }
        else if (address == APU_FRAME_COUNTER) {
            apu->writeFrameCounter(value);
        }
    }
    else { // if (CARTRIDGE_ADDRESSABLE_RANGE.contains(address)) 
        cartridge->mapper->mapPRGWrite(address, value);
    }
}

void Bus::executeCycle() {
    // Three PPU cycles for every CPU cycle
    ppu->executeCycle();
    ppu->executeCycle();
    ppu->executeCycle();

    // TODO: Implement DMA dummy cycle
    if (dmaTransferRequested) {
        doDmaTransferCycle();
    }
    else {
        cpu->executeCycle();
    }

    // Handle interrupt requests
    if (nmiRequest) {
        cpu->NMI();
        nmiRequest = false;
    }
    else if (irqRequest) {
        cpu->IRQ();
        irqRequest = false;
    }

    totalCycles++;
}

void Bus::doDmaTransferCycle() {
    bool cycleMod = totalCycles & 1;

    if (!dmaTransferOngoing && cycleMod == 0) {
        dmaTransferOngoing = true;
    }

    if (dmaTransferOngoing) {
        if (cycleMod == 0) {
            uint16_t dmaAddress = (dmaPage << 8) | dmaOffset;
            dmaData = read(dmaAddress);
        }
        else {
            ppu->oamBuffer[dmaOffset] = dmaData;
            dmaOffset++;
            if (dmaOffset == 0) {
                dmaTransferRequested = false;
                dmaTransferOngoing = false;
            }
        }
    }
}

void Bus::setController(bool controller, Controller::Button button, bool value) {
    controllers[controller].setButton(button, value);
}