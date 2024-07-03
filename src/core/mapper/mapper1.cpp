#include "core/mapper/mapper1.hpp"

#include "util/util.hpp"

Mapper1::Mapper1(uint8_t prgChunks, uint8_t chrChunks, MirrorMode mirrorMode, bool hasBatteryBackedPrgRam, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(prgChunks, chrChunks, mirrorMode, true, prg, chr) { // Mapper 1 has prg ram by default

    shiftRegister = SHIFT_REGISTER_RESET;

    control = 0;
    control.prgRomMode = 0x3;
    control.mirroring = (initialMirrorMode == MirrorMode::HORIZONTAL) ? 0x3 : 0x2;

    chrBank0 = 0;
    chrBank1 = 0;
    prgBank = 0;
}

uint8_t Mapper1::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_ROM_FULL.contains(cpuAddress)) {
        uint32_t mappedAddress;
        if (control.prgRomMode == 0 || control.prgRomMode == 1) {
            // 0, 1: switch 32 KB at $8000
            mappedAddress = (32 * KB) * (prgBank.prgRomSelect >> 1) + (cpuAddress & 0x7FFF);
        }
        else if (control.prgRomMode == 2) {
            // 2: fix first bank at $8000 and switch 16 KB bank at $C000
            if (PRG_ROM_BANK_0.contains(cpuAddress)) {
                mappedAddress = cpuAddress & 0x3FFF;
            }
            else { // if(PRG_ROM_BANK_1.contains(cpuAddress))
                mappedAddress = (16 * KB) * prgBank.prgRomSelect + (cpuAddress & 0x3FFF);
            }
        }
        else { // if (control.prgRomMode == 3)
            // 3: fix last bank at $C000 and switch 16 KB bank at $8000
            if (PRG_ROM_BANK_0.contains(cpuAddress)) {
                mappedAddress = (16 * KB) * prgBank.prgRomSelect + (cpuAddress & 0x3FFF);
            }
            else { // if(PRG_ROM_BANK_1.contains(cpuAddress))
                mappedAddress = (16 * KB) * (prgChunks - 1) + (cpuAddress & 0x3FFF);
            }
        }

        return prg[mappedAddress];
    }
    else if (canAccessPrgRam(cpuAddress) && !prgBank.prgRamDisable) {
        return getPrgRam(cpuAddress);
    }

    return 0;
}

void Mapper1::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (LOAD_REGISTER.contains(cpuAddress)) {
        // TODO: Writes to load register on consecutive cycles are ignored
        if ((value >> 7) & 1) {
            shiftRegister = SHIFT_REGISTER_RESET;
            control.prgRomMode = 0x3;
        }
        else {
            bool done = shiftRegister & 1;

            shiftRegister >>= 1;
            shiftRegister |= ((value & 1) << 4);

            if (done) {
                internalRegisterWrite(cpuAddress, shiftRegister);
                shiftRegister = SHIFT_REGISTER_RESET;
            }
        }
    }
    else if (canAccessPrgRam(cpuAddress) && !prgBank.prgRamDisable) {
        setPrgRam(cpuAddress, value);
    }
}

void Mapper1::internalRegisterWrite(uint16_t address, uint8_t value) {
    if (CONTROL_REGISTER.contains(address)) {
        control = value;
    }
    else if (CHR_REGISTER_0.contains(address)) {
        chrBank0 = value;
    }
    else if (CHR_REGISTER_1.contains(address)) {
        chrBank1 = value;
    }
    else if (PRG_REGISTER.contains(address)) {
        prgBank = value;
    }
}

uint8_t Mapper1::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_ROM_FULL.contains(ppuAddress)) {
        uint32_t mappedAddress;
        if (control.chrRomMode == 0) {
            mappedAddress = (8 * KB) * (chrBank0 >> 1) + (ppuAddress & 0x1FFF);
        }
        else if (CHR_ROM_BANK_0.contains(ppuAddress)) {
            mappedAddress = (4 * KB) * chrBank0 + (ppuAddress & 0x0FFF);
        }
        else { // if(CHR_ROM_BANK_1.contains(ppuAddress))
            mappedAddress = (4 * KB) * chrBank1 + (ppuAddress & 0x0FFF);
        }

        return chr[mappedAddress];
    }

    return 0;
}

void Mapper1::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_ROM_FULL.contains(ppuAddress) && chrChunks == 0) {
        // If chrChunks == 0, we assume we have CHR RAM
        chr[ppuAddress] = value;
    }
}

Mapper::MirrorMode Mapper1::getMirrorMode() const {
    switch (control.mirroring) {
        case 0: return MirrorMode::ONE_SCREEN_LOWER_BANK;
        case 1: return MirrorMode::ONE_SCREEN_UPPER_BANK;
        case 2: return MirrorMode::VERTICAL;
        default: /*case 3:*/ return MirrorMode::HORIZONTAL;
    }
}

Mapper1::Control::Control(uint8_t data) {
    setFromUInt8(data);
}

uint16_t Mapper1::Control::toUInt8() const {
    uint8_t data =
        mirroring |
        (prgRomMode << 2) |
        (chrRomMode << 4);
    return data;
}

void Mapper1::Control::setFromUInt8(uint16_t data) {
    mirroring = data & 0x3;
    prgRomMode = (data >> 2) & 0x3;
    chrRomMode = (data >> 4) & 0x1;
}

Mapper1::PRGBank::PRGBank(uint8_t data) {
    setFromUInt8(data);
}

uint16_t Mapper1::PRGBank::toUInt8() const {
    uint8_t data =
        prgRomSelect |
        (prgRamDisable << 4);
    return data;
}

void Mapper1::PRGBank::setFromUInt8(uint16_t data) {
    prgRomSelect = data & 0xF;
    prgRamDisable = (data >> 4) & 0x1;
}