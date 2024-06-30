#include "core/mapper/mapper66.hpp"

#include "core/cartridge.hpp"

Mapper66::Mapper66(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode)
    : Mapper(prgRomChunks, chrRomChunks, mirrorMode) {

    currentPRGBank = 0;
    currentCHRBank = 0;
}

uint32_t Mapper66::mapToPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        return (Cartridge::PRG_ROM_CHUNK_SIZE << 1) * currentPRGBank + (cpuAddress & 0x7FFF);
    }
    else {
        return 0;
    }
}
uint32_t Mapper66::mapToPRGRead(uint16_t cpuAddress) {
    return mapToPRGView(cpuAddress);
}
uint32_t Mapper66::mapToPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentCHRBank = value & 0x3;
        currentPRGBank = (value >> 4) & 0x3;
    }

    // PRG in mapper 66 is read only
    return 0;
}

uint32_t Mapper66::mapToCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return Cartridge::CHR_ROM_CHUNK_SIZE * currentCHRBank + (ppuAddress & 0x1FFF);
    }
    else {
        return 0;
    }
}
uint32_t Mapper66::mapToCHRRead(uint16_t ppuAddress) {
    return mapToCHRView(ppuAddress);
}
uint32_t Mapper66::mapToCHRWrite(uint16_t /*ppuAddress*/, uint8_t /*value*/) {
    // CHR in mapper 66 is read only
    return 0;
}