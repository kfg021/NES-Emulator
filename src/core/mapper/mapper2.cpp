#include "core/mapper/mapper2.hpp"

#include "core/cartridge.hpp"

Mapper2::Mapper2(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(config, prg, chr) {

    currentBank = 0;
}

uint8_t Mapper2::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE_SWICHABLE.contains(cpuAddress)) {
        uint32_t mappedAddress = PRG_ROM_CHUNK_SIZE * currentBank + (cpuAddress & 0x3FFF);
        return prg[mappedAddress];
    }
    else if (PRG_RANGE_FIXED.contains(cpuAddress)) {
        uint32_t mappedAddress = PRG_ROM_CHUNK_SIZE * (config.prgChunks - 1) + (cpuAddress & 0x3FFF);
        return prg[mappedAddress];
    }
    else if (canAccessPrgRam(cpuAddress)) {
        return getPrgRam(cpuAddress);
    }

    return 0;
}

void Mapper2::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value & 0x7;
    }
    else if (canAccessPrgRam(cpuAddress)) {
        setPrgRam(cpuAddress, value);
    }
}

uint8_t Mapper2::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[ppuAddress];
    }

    return 0;
}

void Mapper2::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_RANGE.contains(ppuAddress) && config.chrChunks == 0) {
        // If chrRomChunks == 0, we assume we have CHR RAM
        chr[ppuAddress] = value;
    }
}