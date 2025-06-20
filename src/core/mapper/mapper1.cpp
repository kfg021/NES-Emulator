#include "core/mapper/mapper1.hpp"

#include "util/util.hpp"

Mapper1::Mapper1(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(config, prg, chr),
    prgRam(true), // Mapper 1 has PRG RAM by default
    chrRam(config.chrChunks == 0) {

    reset();
}

void Mapper1::reset() {
    shiftRegister = SHIFT_REGISTER_RESET;

    control.mirroring = (config.initialMirrorMode == MirrorMode::HORIZONTAL) ? 0x3 : 0x2;
    control.prgRomMode = 0x3;
    control.chrRomMode = 0;

    chrBank0 = 0;
    chrBank1 = 0;

    prgBank.data = 0;

    if (!config.hasBatteryBackedPrgRam) {
        prgRam.reset();
    }
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
    else if (!prgBank.prgRamDisable) {
        return prgRam.tryRead(cpuAddress).value_or(0);
    }
    else {
        return 0;
    }
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
    else if (!prgBank.prgRamDisable) {
        prgRam.tryWrite(cpuAddress, value);
    }
}

void Mapper1::internalRegisterWrite(uint16_t address, uint8_t value) {
    if (CONTROL_REGISTER.contains(address)) {
        control.data = value;
    }
    else if (CHR_REGISTER_0.contains(address)) {
        chrBank0 = value;
    }
    else if (CHR_REGISTER_1.contains(address)) {
        chrBank1 = value;
    }
    else if (PRG_REGISTER.contains(address)) {
        prgBank.data = value;
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

        return readChrRomOrRam(mappedAddress, chr, chrRam);
    }

    return 0;
}

void Mapper1::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    chrRam.tryWrite(ppuAddress, value);
}

Mapper::MirrorMode Mapper1::getMirrorMode() const {
    switch (control.mirroring) {
        case 0: return MirrorMode::ONE_SCREEN_LOWER_BANK;
        case 1: return MirrorMode::ONE_SCREEN_UPPER_BANK;
        case 2: return MirrorMode::VERTICAL;
        default: /*case 3:*/ return MirrorMode::HORIZONTAL;
    }
}

void Mapper1::serialize(Serializer& s) const {
    s.serializeUInt8(shiftRegister);
    s.serializeUInt8(control.data);
    s.serializeUInt8(chrBank0);
    s.serializeUInt8(chrBank1);
    s.serializeUInt8(prgBank.data);
    s.serializeVector(prgRam.data, s.uInt8Func);
    if (chrRam.isEnabled) {
        s.serializeVector(chrRam.data, s.uInt8Func);
    }
}

void Mapper1::deserialize(Deserializer& d) {
    d.deserializeUInt8(shiftRegister);
    d.deserializeUInt8(control.data);
    d.deserializeUInt8(chrBank0);
    d.deserializeUInt8(chrBank1);
    d.deserializeUInt8(prgBank.data);
    d.deserializeVector(prgRam.data, d.uInt8Func);
    if (chrRam.isEnabled) {
        d.deserializeVector(chrRam.data, d.uInt8Func);
    }
}