#include "core/mapper/mapper66.hpp"

#include "core/cartridge.hpp"

Mapper66::Mapper66(uint8_t prgRomChunks, uint8_t chrRomChunks)
    : Mapper(prgRomChunks, chrRomChunks) {
    
    currentPRGBank = 0;
    currentCHRBank = 0;
}

std::optional<uint32_t> Mapper66::mapToPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        return (Cartridge::PRG_ROM_CHUNK_SIZE << 1) * currentPRGBank + (cpuAddress & 0x7FFF);
    }
    else {
        return std::nullopt;
    }
}
std::optional<uint32_t> Mapper66::mapToPRGRead(uint16_t cpuAddress) {
    return mapToPRGView(cpuAddress);
}
std::optional<uint32_t> Mapper66::mapToPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentCHRBank = value & 0x3;
        currentPRGBank = (value >> 4) & 0x3;
    }

    // PRG in mapper 66 is read only
    return std::nullopt;
}

std::optional<uint32_t> Mapper66::mapToCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return Cartridge::CHR_ROM_CHUNK_SIZE * currentCHRBank + (ppuAddress & 0x1FFF);
    }
    else {
        return std::nullopt;
    }
}
std::optional<uint32_t> Mapper66::mapToCHRRead(uint16_t ppuAddress) {
    return mapToCHRView(ppuAddress);
}
std::optional<uint32_t> Mapper66::mapToCHRWrite(uint16_t /*ppuAddress*/, uint8_t /*value*/) {
    // CHR in mapper 66 is read only
    return std::nullopt;
}