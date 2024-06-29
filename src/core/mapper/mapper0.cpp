#include "core/mapper/mapper0.hpp"

Mapper0::Mapper0(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode)
    : Mapper(prgRomChunks, chrRomChunks, mirrorMode) {
}

std::optional<uint32_t> Mapper0::mapToPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (prgRomChunks == 1) {
            return cpuAddress & 0x3FFF;
        }
        else if (prgRomChunks == 2) {
            return cpuAddress & 0x7FFF;
        }
        else {
            return std::nullopt;
        }
    }
    else {
        return std::nullopt;
    }
}

std::optional<uint32_t> Mapper0::mapToPRGRead(uint16_t cpuAddress) {
    return mapToPRGView(cpuAddress);
}

std::optional<uint32_t> Mapper0::mapToPRGWrite(uint16_t /*cpuAddress*/, uint8_t /*value*/) {
    // PRG in mapper 0 is read only
    return std::nullopt;
}


std::optional<uint32_t> Mapper0::mapToCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return ppuAddress;
    }
    else {
        return std::nullopt;
    }
}

std::optional<uint32_t> Mapper0::mapToCHRRead(uint16_t ppuAddress) {
    return mapToCHRView(ppuAddress);
}

std::optional<uint32_t> Mapper0::mapToCHRWrite(uint16_t ppuAddress, uint8_t /*value*/) {
    if (CHR_RANGE.contains(ppuAddress) && chrRomChunks == 0) {
        // If chrRomChunks == 0, we assume we have CHR RAM
        return ppuAddress;
    }
    else {
        return std::nullopt;
    }
}