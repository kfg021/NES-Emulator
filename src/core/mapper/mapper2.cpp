#include "core/mapper/mapper2.hpp"

#include "core/cartridge.hpp"

Mapper2::Mapper2(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode)
    : Mapper(prgRomChunks, chrRomChunks, mirrorMode) {

    currentBank = 0;
}

std::optional<uint32_t> Mapper2::mapToPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE_SWICHABLE.contains(cpuAddress)) {
        return Cartridge::PRG_ROM_CHUNK_SIZE * currentBank + (cpuAddress & 0x3FFF);
    }
    else if (PRG_RANGE_FIXED.contains(cpuAddress)) {
        return Cartridge::PRG_ROM_CHUNK_SIZE * (prgRomChunks - 1) + (cpuAddress & 0x3FFF);
    }
    else {
        return std::nullopt;
    }
}
std::optional<uint32_t> Mapper2::mapToPRGRead(uint16_t cpuAddress) {
    return mapToPRGView(cpuAddress);
}
std::optional<uint32_t> Mapper2::mapToPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value & 0x7;
    }

    // PRG in mapper 2 is read only
    return std::nullopt;
}

std::optional<uint32_t> Mapper2::mapToCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return ppuAddress;
    }
    else {
        return std::nullopt;
    }
}
std::optional<uint32_t> Mapper2::mapToCHRRead(uint16_t ppuAddress) {
    return mapToCHRView(ppuAddress);
}
std::optional<uint32_t> Mapper2::mapToCHRWrite(uint16_t ppuAddress, uint8_t /*value*/) {
    if (CHR_RANGE.contains(ppuAddress) && chrRomChunks == 0) {
        // If chrRomChunks == 0, we assume we have CHR RAM
        return ppuAddress;
    }
    else {
        return std::nullopt;
    }
}