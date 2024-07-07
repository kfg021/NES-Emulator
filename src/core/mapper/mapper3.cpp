#include "core/mapper/mapper3.hpp"

#include "core/cartridge.hpp"

Mapper3::Mapper3(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(config, prg, chr) {

    currentBank = 0;
}

uint8_t Mapper3::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (config.prgChunks == 1) {
            return prg[cpuAddress & 0x3FFF];
        }
        else if (config.prgChunks == 2) {
            return prg[cpuAddress & 0x7FFF];
        }
    }
    else if (canAccessPrgRam(cpuAddress)) {
        return getPrgRam(cpuAddress);
    }

    return 0;
}

void Mapper3::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value;
    }
    else if (canAccessPrgRam(cpuAddress)) {
        setPrgRam(cpuAddress, value);
    }
}

uint8_t Mapper3::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[CHR_ROM_CHUNK_SIZE * currentBank + ppuAddress];
    }

    return 0;
}

void Mapper3::mapCHRWrite(uint16_t /*ppuAddress*/, uint8_t /*value*/) {
    // CHR in mapper 3 is read only
}