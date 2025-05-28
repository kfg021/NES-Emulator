#include "core/bus.hpp"

Bus::Bus() {
    cpu = std::make_unique<CPU>();
    cpu->setBus(this);

    ppu = std::make_unique<PPU>();

    apu = std::make_unique<APU>();
    apu->setBus(this);
}

void Bus::initDevices() {
    ram = {};

    controllers = {};
    controllerData = {};
    strobe = 0;

    cpu->initCPU();
    ppu->initPPU();
    apu->initAPU();

    totalCycles = 0;

    oamDma = {};
    dmcDma = {};
}

Cartridge::Status Bus::loadROM(const std::string& filePath) {
    ppu->cartridge = Cartridge(filePath);

    const Cartridge::Status& status = ppu->cartridge.getStatus();
    if (status.code == Cartridge::Code::SUCCESS) {
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
        return ppu->cartridge.mapper->mapPRGView(address);
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
        return ppu->cartridge.mapper->mapPRGRead(address);
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
            oamDma.requested = true;
            oamDma.page = value;
        }
        else if (address == APU_STATUS) {
            apu->writeStatus(value);
        }
        else if (address == APU_FRAME_COUNTER) {
            apu->writeFrameCounter(value);
        }
    }
    else { // if (CARTRIDGE_ADDRESSABLE_RANGE.contains(address)) 
        ppu->cartridge.mapper->mapPRGWrite(address, value);
    }
}

void Bus::executeCycle() {
    // Three PPU cycles for every CPU cycle
    ppu->executeCycle();
    ppu->executeCycle();
    ppu->executeCycle();

    // Handle DMA transfers
    if (oamDma.requested) {
        oamDmaCycle();
    }
    else if (dmcDma.requested) {
        dmcDmaCycle();
    }
    else {
        cpu->executeCycle();
    }

    // 2 CPU Cycles for every APU cycle
    apu->executeHalfCycle();

    bool nmiRequested = ppu->nmiRequested();
    bool irqRequested = ppu->irqRequested() || apu->irqRequested();

    // Handle interrupt requests
    if (nmiRequested) {
        cpu->NMI();
        ppu->clearNMIRequest();
    }

    if (irqRequested) {
        cpu->IRQ();
    }

    totalCycles++;
}

void Bus::oamDmaCycle() {
    bool cycleMod = totalCycles & 1;

    if (!oamDma.ongoing && cycleMod == 0) {
        oamDma.ongoing = true;
    }

    if (oamDma.ongoing) {
        if (cycleMod == 0) {
            uint16_t dmaAddress = (oamDma.page << 8) | oamDma.offset;
            oamDma.data = read(dmaAddress);
        }
        else {
            ppu->oamBuffer[oamDma.offset] = oamDma.data;
            oamDma.offset++;
            if (oamDma.offset == 0) {
                oamDma.requested = false;
                oamDma.ongoing = false;
            }
        }
    }
}

void Bus::dmcDmaCycle() {
    // DMC DMA takes 4 cycles (3 stall cycles + 1 read)
    if (!dmcDma.ongoing) {
        dmcDma.ongoing = true;
        dmcDma.delay = 0;
    }

    // Count cycles
    dmcDma.delay++;

    // On the 4th cycle, read the data and pass to DMC
    if (dmcDma.delay >= 4) {
        dmcDma.data = read(dmcDma.address);
        apu->receiveDMCSample(dmcDma.data);
        dmcDma.requested = false;
        dmcDma.ongoing = false;
        dmcDma.delay = 0;
    }
}

void Bus::requestDmcDma(uint16_t address) {
    dmcDma.requested = true;
    dmcDma.address = address;
}

void Bus::setController(bool controller, uint8_t value) {
    controllers[controller].setButtons(value);
}

Bus::State Bus::getState() const {
    State state = {
        totalCycles,
        ram,
        controllerData,
        strobe,
        oamDma,
        dmcDma
    };
    return state;
}
void Bus::restoreState(const Bus::State& state) {
    totalCycles = state.totalCycles;
    ram = state.ram;
    controllerData = state.controllerData;
    strobe = state.strobe;
    oamDma = state.oamDma;
    dmcDma = state.dmcDma;
}