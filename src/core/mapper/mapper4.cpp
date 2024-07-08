#include "core/mapper/mapper4.hpp"

Mapper::Config editConfigMapper4(Mapper::Config config) {
    // Mapper 4 has PRG RAM by default
    config.hasBatteryBackedPrgRam = true;

    if (config.alternativeNametableLayout) {
        config.initialMirrorMode = Mapper::MirrorMode::FOUR_SCREEN;
    }

    return config;
}

Mapper4::Mapper4(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(editConfigMapper4(config), prg, chr) {
    bankSelect = 0;
    bankData = 0;
    mirroring = (config.initialMirrorMode == MirrorMode::HORIZONTAL);
    prgRamProtect = 0;
    irqReloadValue = 0;
    irqTimer = 0;
    irqEnabled = 0;

    prgSwitchableBankSelect = {};
    chrSwitchableBankSelect = {};

    if (config.alternativeNametableLayout) {
        customNametable = std::vector<uint8_t>(4 * KB, 0);
    }
}

bool Mapper4::irqRequestAtEndOfScanline() {
    if (irqTimer == 0) {
        irqTimer = irqReloadValue;
    }
    else {
        irqTimer--;
    }

    return irqEnabled && (irqTimer == 0);
}

uint8_t Mapper4::mapPRGView(uint16_t cpuAddress) const {
    bool prgRomBankMode = (bankSelect >> 6) & 1;
    uint16_t addressMask8KB = cpuAddress & MASK<8 * KB>();
    uint16_t prgChunks8KB = config.prgChunks << 1;

    if (PRG_RANGE.contains(cpuAddress)) {
        uint32_t mappedAddress;
        if (PRG_ROM_8KB_SWITCHABLE_1[prgRomBankMode].contains(cpuAddress)) {
            mappedAddress = (8 * KB) * prgSwitchableBankSelect[0] + addressMask8KB;
        }
        else if (PRG_ROM_8KB_SWITCHABLE_2.contains(cpuAddress)) {
            mappedAddress = (8 * KB) * prgSwitchableBankSelect[1] + addressMask8KB;
        }
        else if (PRG_ROM_8KB_FIXED_1[prgRomBankMode].contains(cpuAddress)) {
            mappedAddress = (8 * KB) * (prgChunks8KB - 2) + addressMask8KB;
        }
        else { // if (PRG_ROM_8KB_FIXED_2.contains(cpuAddress)) {
            mappedAddress = (8 * KB) * (prgChunks8KB - 1) + addressMask8KB;
        }
        return prg[mappedAddress];
    }
    else if (canAccessPrgRam(cpuAddress) && canReadFromPRGRam()) {
        return getPrgRam(cpuAddress);
    }

    return 0;
}

void Mapper4::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_OR_BANK_DATA.contains(cpuAddress)) {
        if ((cpuAddress & 1) == 0) {
            bankSelect = value;
        }
        else {
            uint8_t bankRegister = bankSelect & 0x7;

            if (/*bankRegister >= 0 &&*/ bankRegister < 6) {
                chrSwitchableBankSelect[bankRegister] = value;
            }
            else { // if(bankRegister >= 6 && bankRegister <= 7) {
                prgSwitchableBankSelect[bankRegister & 1] = value & 0x3F;
            }
        }
    }
    else if (MIRRORING_OR_PRG_RAM_PROTECT.contains(cpuAddress)) {
        if ((cpuAddress & 1) == 0) {
            mirroring = value & 1;
        }
        else {
            prgRamProtect = value;
        }
    }
    else if (IRQ_LATCH_OR_IRQ_RELOAD.contains(cpuAddress)) {
        if ((cpuAddress & 1) == 0) {
            irqReloadValue = value;
        }
        else {
            irqTimer = 0;
        }
    }
    else if (IRQ_DISABLE_OR_IRQ_ENABLE.contains(cpuAddress)) {
        if ((cpuAddress & 1) == 0) {
            irqEnabled = false;
        }
        else {
            irqEnabled = true;
        }
    }
    else if (canAccessPrgRam(cpuAddress) && canWriteToPRGRam()) {
        setPrgRam(cpuAddress, value);
    }
}

uint8_t Mapper4::mapCHRView(uint16_t ppuAddress) const {
    bool chrRomBankMode = (bankSelect >> 7) & 1;
    uint16_t addressMask2KB = ppuAddress & MASK<2 * KB>();
    uint16_t addressMaskKB = ppuAddress & MASK<KB>();

    if (CHR_RANGE.contains(ppuAddress)) {
        uint32_t mappedAddress;
        if (CHR_ROM_2KB_SWITCHABLE_1[chrRomBankMode].contains(ppuAddress)) {
            mappedAddress = (2 * KB) * (chrSwitchableBankSelect[0] >> 1) + addressMask2KB;
        }
        else if (CHR_ROM_2KB_SWITCHABLE_2[chrRomBankMode].contains(ppuAddress)) {
            mappedAddress = (2 * KB) * (chrSwitchableBankSelect[1] >> 1) + addressMask2KB;
        }
        else if (CHR_ROM_1KB_SWITCHABLE_1[chrRomBankMode].contains(ppuAddress)) {
            mappedAddress = KB * chrSwitchableBankSelect[2] + addressMaskKB;
        }
        else if (CHR_ROM_1KB_SWITCHABLE_2[chrRomBankMode].contains(ppuAddress)) {
            mappedAddress = KB * chrSwitchableBankSelect[3] + addressMaskKB;
        }
        else if (CHR_ROM_1KB_SWITCHABLE_3[chrRomBankMode].contains(ppuAddress)) {
            mappedAddress = KB * chrSwitchableBankSelect[4] + addressMaskKB;
        }
        else { // if (CHR_ROM_1KB_SWITCHABLE_4[chrRomBankMode].contains(ppuAddress)) {
            mappedAddress = KB * chrSwitchableBankSelect[5] + addressMaskKB;
        }
        return chr[mappedAddress];
    }
    else if (config.alternativeNametableLayout) {
        if (ALTERNATIVE_NAMETABLE_RANGE.contains(ppuAddress)) {
            return customNametable[ppuAddress & MASK<4 * KB>()];
        }
    }

    return 0;
}

void Mapper4::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (config.alternativeNametableLayout) {
        if (ALTERNATIVE_NAMETABLE_RANGE.contains(ppuAddress)) {
            customNametable[ppuAddress & MASK<4 * KB>()] = value;
        }
    }
}

Mapper::MirrorMode Mapper4::getMirrorMode() const {
    if (!config.alternativeNametableLayout) {
        return mirroring ? MirrorMode::HORIZONTAL : MirrorMode::VERTICAL;
    }
    else {
        return config.initialMirrorMode;
    }
}

bool Mapper4::canReadFromPRGRam() const {
    bool prgRamChipEnable = (prgRamProtect >> 7) & 1;
    return prgRamChipEnable;
}

bool Mapper4::canWriteToPRGRam() const {
    bool writeProtection = (prgRamProtect >> 6) & 1;
    return canReadFromPRGRam() && !writeProtection;
}