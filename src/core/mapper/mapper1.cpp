#include "core/mapper/mapper1.hpp"

#include "util/util.hpp"

static Mapper::Config editConfigMapper1(Mapper::Config config) {
    // Mapper 1 has PRG RAM by default
    config.hasBatteryBackedPrgRam = true;
    return config;
}

Mapper1::Mapper1(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(editConfigMapper1(config), prg, chr) {
    reset();
}

void Mapper1::reset() {
    shiftRegister = SHIFT_REGISTER_RESET;

    control = 0;
    control.prgRomMode = 0x3;
    control.mirroring = (config.initialMirrorMode == MirrorMode::HORIZONTAL) ? 0x3 : 0x2;

    chrBank0 = 0;
    chrBank1 = 0;
    prgBank = 0;
}

uint8_t Mapper1::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        uint8_t prgRomSelect = prgBank.prgRomSelect % config.prgChunks;

        uint32_t mappedAddress;
        if (control.prgRomMode == 0 || control.prgRomMode == 1) {
            // 0, 1: switch 32 KB at $8000
            mappedAddress = (32 * KB) * (prgRomSelect >> 1) + (cpuAddress & MASK<32 * KB>());
        }
        else if (control.prgRomMode == 2) {
            // 2: fix first bank at $8000 and switch 16 KB bank at $C000
            if (PRG_ROM_BANK_0.contains(cpuAddress)) {
                mappedAddress = cpuAddress & MASK<16 * KB>();
            }
            else { // if (PRG_ROM_BANK_1.contains(cpuAddress))
                mappedAddress = (16 * KB) * prgRomSelect + (cpuAddress & MASK<16 * KB>());
            }
        }
        else { // if (control.prgRomMode == 3)
            // 3: fix last bank at $C000 and switch 16 KB bank at $8000
            if (PRG_ROM_BANK_0.contains(cpuAddress)) {
                mappedAddress = (16 * KB) * prgRomSelect + (cpuAddress & MASK<16 * KB>());
            }
            else { // if (PRG_ROM_BANK_1.contains(cpuAddress))
                mappedAddress = (16 * KB) * (config.prgChunks - 1) + (cpuAddress & MASK<16 * KB>());
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
        // TODO: If two writes occur on consecutive cycles, the second one should be ignored
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
    if (CHR_RANGE.contains(ppuAddress)) {
        uint32_t mappedAddress;
        if (control.chrRomMode == 0) {
            mappedAddress = (8 * KB) * (chrBank0 >> 1) + (ppuAddress & MASK<8 * KB>());
        }
        else if (CHR_ROM_BANK_0.contains(ppuAddress)) {
            mappedAddress = (4 * KB) * chrBank0 + (ppuAddress & MASK<4 * KB>());
        }
        else { // if (CHR_ROM_BANK_1.contains(ppuAddress))
            mappedAddress = (4 * KB) * chrBank1 + (ppuAddress & MASK<4 * KB>());
        }

        return chr[mappedAddress];
    }

    return 0;
}

void Mapper1::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_RANGE.contains(ppuAddress) && hasChrRam()) {
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

uint8_t Mapper1::Control::toUInt8() const {
    uint8_t data =
        mirroring |
        (prgRomMode << 2) |
        (chrRomMode << 4);
    return data;
}

void Mapper1::Control::setFromUInt8(uint8_t data) {
    mirroring = data & 0x3;
    prgRomMode = (data >> 2) & 0x3;
    chrRomMode = (data >> 4) & 0x1;
}

Mapper1::PRGBank::PRGBank(uint8_t data) {
    setFromUInt8(data);
}

uint8_t Mapper1::PRGBank::toUInt8() const {
    uint8_t data =
        prgRomSelect |
        (prgRamDisable << 4);
    return data;
}

void Mapper1::PRGBank::setFromUInt8(uint8_t data) {
    prgRomSelect = data & 0xF;
    prgRamDisable = (data >> 4) & 0x1;
}

bool Mapper1::hasChrRam() const {
    // If chrChunks == 0, we assume we have CHR RAM
    return config.chrChunks == 0;
}

void Mapper1::serialize(Serializer& s) const {
    s.serializeUInt8(shiftRegister);
    s.serializeUInt8(control.toUInt8());
    s.serializeUInt8(chrBank0);
    s.serializeUInt8(chrBank1);
    s.serializeUInt8(prgBank.toUInt8());
    s.serializeVector(prgRam, s.uInt8Func);
    if (hasChrRam()) {
        s.serializeVector(chr, s.uInt8Func);
    }
}

void Mapper1::deserialize(Deserializer& d) {
    d.deserializeUInt8(shiftRegister);

    uint8_t controlTemp;
    d.deserializeUInt8(controlTemp);
    control.setFromUInt8(controlTemp);

    d.deserializeUInt8(chrBank0);
    d.deserializeUInt8(chrBank1);

    uint8_t prgBankTemp;
    d.deserializeUInt8(prgBankTemp);
    prgBank.setFromUInt8(prgBankTemp);

    d.deserializeVector(prgRam, d.uInt8Func);
    if (hasChrRam()) {
        d.deserializeVector(chr, d.uInt8Func);
    }
}