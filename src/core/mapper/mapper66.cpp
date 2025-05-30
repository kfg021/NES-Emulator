#include "core/mapper/mapper66.hpp"

#include "core/cartridge.hpp"

Mapper66::Mapper66(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(config, prg, chr) {

    currentPRGBank = 0;
    currentCHRBank = 0;
}

uint8_t Mapper66::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        uint32_t mappedAddress = (PRG_ROM_CHUNK_SIZE << 1) * currentPRGBank + (cpuAddress & MASK<32 * KB>());
        return prg[mappedAddress];
    }
    else if (canAccessPrgRam(cpuAddress)) {
        return getPrgRam(cpuAddress);
    }

    return 0;
}

void Mapper66::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentCHRBank = value & 0x3;
        currentPRGBank = (value >> 4) & 0x3;
    }
    else if (canAccessPrgRam(cpuAddress)) {
        setPrgRam(cpuAddress, value);
    }
}

uint8_t Mapper66::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        uint32_t mappedAddress = CHR_ROM_CHUNK_SIZE * currentCHRBank + (ppuAddress & MASK<8 * KB>());
        return chr[mappedAddress];
    }

    return 0;
}

void Mapper66::mapCHRWrite(uint16_t /*ppuAddress*/, uint8_t /*value*/) {
    // CHR in mapper 66 is read only
}

Mapper66::State Mapper66::getState() const {
    State state = {
        currentPRGBank,
        currentCHRBank,
        prgRam
    };
    return state;
}

void Mapper66::restoreState(const Mapper66::State& state) {
    currentPRGBank = state.currentPRGBank;
    currentCHRBank = state.currentCHRBank;
    prgRam = state.prgRam;
}