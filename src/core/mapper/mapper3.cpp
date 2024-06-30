#include "core/mapper/mapper3.hpp"

#include "core/cartridge.hpp"

Mapper3::Mapper3(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode)
    : Mapper(prgRomChunks, chrRomChunks, mirrorMode) {

    currentBank = 0;
}

uint32_t Mapper3::mapToPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (prgRomChunks == 1) {
            return cpuAddress & 0x3FFF;
        }
        else if (prgRomChunks == 2) {
            return cpuAddress & 0x7FFF;
        }
    }
    
    return 0;
}
uint32_t Mapper3::mapToPRGRead(uint16_t cpuAddress) {
    return mapToPRGView(cpuAddress);
}
uint32_t Mapper3::mapToPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value;
    }

    // PRG in mapper 3 is read only
    return 0;
}

uint32_t Mapper3::mapToCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return Cartridge::CHR_ROM_CHUNK_SIZE * currentBank + ppuAddress;
    }
    else {
        return 0;
    }
}
uint32_t Mapper3::mapToCHRRead(uint16_t ppuAddress) {
    return mapToCHRView(ppuAddress);
}
uint32_t Mapper3::mapToCHRWrite(uint16_t /*ppuAddress*/, uint8_t /*value*/) {
    // CHR in mapper 3 is read only
    return 0;
}