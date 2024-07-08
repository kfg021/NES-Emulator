#include "core/mapper/mapper9.hpp"

Mapper9::Mapper9(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(config, prg, chr) {

    prgBankSelect = 0;
    chrLatch1 = 0;
    chrLatch2 = 0;
    chrBank1Select = {};
    chrBank2Select = {};

    mirroring = (config.initialMirrorMode == MirrorMode::HORIZONTAL);
}

uint8_t Mapper9::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        uint32_t mappedAddress;
        if (PRG_ROM_SWITCHABLE.contains(cpuAddress)) {
            mappedAddress = (8 * KB) * prgBankSelect + (cpuAddress & MASK<8 * KB>());
        }
        else { // if (PRG_ROM_FIXED.contains(cpuAddress)) {
            // 3 8KB chunks fixed to the last 3 banks
            uint16_t prgChunks8KB = config.prgChunks << 1;
            mappedAddress = (8 * KB) * (prgChunks8KB - 3) + (cpuAddress - PRG_ROM_FIXED.lo);
        }
        return prg[mappedAddress];
    }

    return 0;
}

void Mapper9::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (PRG_ROM_BANK_SELECT.contains(cpuAddress)) {
        prgBankSelect = value & 0xF;
    }
    else if (CHR_ROM_BANK_1_SELECT_OPTION_1.contains(cpuAddress)) {
        chrBank1Select[0] = value & 0x1F;
    }
    else if (CHR_ROM_BANK_1_SELECT_OPTION_2.contains(cpuAddress)) {
        chrBank1Select[1] = value & 0x1F;
    }
    else if (CHR_ROM_BANK_2_SELECT_OPTION_1.contains(cpuAddress)) {
        chrBank2Select[0] = value & 0x1F;
    }
    else if (CHR_ROM_BANK_2_SELECT_OPTION_2.contains(cpuAddress)) {
        chrBank2Select[1] = value & 0x1F;
    }
    else if (MIRRORING.contains(cpuAddress)) {
        mirroring = value & 0x1;
    }
}

uint8_t Mapper9::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        uint32_t mappedAddress;
        if (CHR_ROM_SWITCHABLE_1.contains(ppuAddress)) {
            mappedAddress = (4 * KB) * chrBank1Select[chrLatch1] + (ppuAddress & MASK<4 * KB>());
        }
        else { // if (CHR_ROM_SWITCHABLE_2.contains(ppuAddress)) {
            mappedAddress = (4 * KB) * chrBank2Select[chrLatch2] + (ppuAddress & MASK<4 * KB>());
        }
        return chr[mappedAddress];
    }

    return 0;
}

uint8_t Mapper9::mapCHRRead(uint16_t ppuAddress) {
    if (CHR_RANGE.contains(ppuAddress)) {
        uint32_t mappedAddress;
        if (CHR_ROM_SWITCHABLE_1.contains(ppuAddress)) {
            mappedAddress = (4 * KB) * chrBank1Select[chrLatch1] + (ppuAddress & MASK<4 * KB>());
        }
        else { // if (CHR_ROM_SWITCHABLE_2.contains(ppuAddress)) {
            mappedAddress = (4 * KB) * chrBank2Select[chrLatch2] + (ppuAddress & MASK<4 * KB>());
        }

        if (ppuAddress == LATCH_1_DISABLE) {
            chrLatch1 = false;
        }
        else if (ppuAddress == LATCH_1_ENABLE) {
            chrLatch1 = true;
        }
        else if (LATCH_2_DISABLE.contains(ppuAddress)) {
            chrLatch2 = false;
        }
        else if (LATCH_2_ENABLE.contains(ppuAddress)) {
            chrLatch2 = true;
        }

        return chr[mappedAddress];
    }

    return 0;
}

void Mapper9::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    // Mapper 9 CHR is read only
}

Mapper::MirrorMode Mapper9::getMirrorMode() const {
    return mirroring ? MirrorMode::HORIZONTAL : MirrorMode::VERTICAL;
}