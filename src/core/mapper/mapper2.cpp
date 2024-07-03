#include "core/mapper/mapper2.hpp"

#include "core/cartridge.hpp"

Mapper2::Mapper2(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(prgRomChunks, chrRomChunks, mirrorMode, prg, chr) {

    currentBank = 0;
}

uint8_t Mapper2::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE_SWICHABLE.contains(cpuAddress)) {
        uint32_t mappedAddress = PRG_ROM_CHUNK_SIZE * currentBank + (cpuAddress & 0x3FFF);
        return prg[mappedAddress];
    }
    else if (PRG_RANGE_FIXED.contains(cpuAddress)) {
        uint32_t mappedAddress = PRG_ROM_CHUNK_SIZE * (prgChunks - 1) + (cpuAddress & 0x3FFF);
        return prg[mappedAddress];
    }
    else {
        return 0;
    }
}

void Mapper2::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value & 0x7;
    }

    // PRG in mapper 2 is read only
}

uint8_t Mapper2::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[ppuAddress];
    }
    else {
        return 0;
    }
}

void Mapper2::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_RANGE.contains(ppuAddress) && chrChunks == 0) {
        // If chrRomChunks == 0, we assume we have CHR RAM
        chr[ppuAddress] = value;
    }
}