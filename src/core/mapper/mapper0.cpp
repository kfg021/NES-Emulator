#include "core/mapper/mapper0.hpp"

Mapper0::Mapper0(uint8_t prgChunks, uint8_t chrChunks, MirrorMode mirrorMode, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(prgChunks, chrChunks, mirrorMode, prg, chr) {
}

uint8_t Mapper0::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (prgChunks == 1) {
            return prg[cpuAddress & 0x3FFF];
        }
        else if (prgChunks == 2) {
            return prg[cpuAddress & 0x7FFF];
        }
    }

    return 0;
}

void Mapper0::mapPRGWrite(uint16_t /*cpuAddress*/, uint8_t /*value*/) {
    // PRG in mapper 0 is read only
}

uint8_t Mapper0::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[ppuAddress];
    }
    else {
        return 0;
    }
}

void Mapper0::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_RANGE.contains(ppuAddress) && chrChunks == 0) {
        // If chrChunks == 0, we assume we have CHR RAM
        chr[ppuAddress] = value;
    }
}