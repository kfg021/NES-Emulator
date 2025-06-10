#include "core/mapper/mapper0.hpp"

Mapper0::Mapper0(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(config, prg, chr),
    prgRam(config.hasBatteryBackedPrgRam),
    chrRam(config.chrChunks == 0) {
}

void Mapper0::reset() {
    // Mapper 0 has no state
}

uint8_t Mapper0::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (config.prgChunks == 1) {
            return prg[cpuAddress & MASK<16 * KB>()];
        }
        else if (config.prgChunks == 2) {
            return prg[cpuAddress & MASK<32 * KB>()];
        }
        else {
            return 0;
        }
    }
    else {
        return prgRam.tryRead(cpuAddress).value_or(0);
    }
}

void Mapper0::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    prgRam.tryWrite(cpuAddress, value);
}

uint8_t Mapper0::mapCHRView(uint16_t ppuAddress) const {
    return readChrRomOrRam(ppuAddress, chr, chrRam);
}

void Mapper0::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    chrRam.tryWrite(ppuAddress, value);
}

void Mapper0::serialize(Serializer& s) const {
    s.serializeVector(prgRam.data, s.uInt8Func);
    if (chrRam.isEnabled) {
        s.serializeVector(chrRam.data, s.uInt8Func);
    }
}

void Mapper0::deserialize(Deserializer& d) {
    d.deserializeVector(prgRam.data, d.uInt8Func);
    if (chrRam.isEnabled) {
        d.deserializeVector(chrRam.data, d.uInt8Func);
    }
}