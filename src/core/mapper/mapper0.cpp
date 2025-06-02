#include "core/mapper/mapper0.hpp"

Mapper0::Mapper0(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(config, prg, chr) {
}

uint8_t Mapper0::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (config.prgChunks == 1) {
            return prg[cpuAddress & MASK<16 * KB>()];
        }
        else if (config.prgChunks == 2) {
            return prg[cpuAddress & MASK<32 * KB>()];
        }
    }
    else if (canAccessPrgRam(cpuAddress)) {
        return getPrgRam(cpuAddress);
    }

    return 0;
}

void Mapper0::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (canAccessPrgRam(cpuAddress)) {
        setPrgRam(cpuAddress, value);
    }
}

uint8_t Mapper0::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[ppuAddress];
    }

    return 0;
}

void Mapper0::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_RANGE.contains(ppuAddress) && hasChrRam()) {
        chr[ppuAddress] = value;
    }
}

bool Mapper0::hasChrRam() const {
    // If chrChunks == 0, we assume we have CHR RAM
    return config.chrChunks == 0;
}

void Mapper0::serialize(Serializer& s) const {
    s.serializeVector(prgRam, s.uInt8Func);
    if (hasChrRam()) {
        s.serializeVector(chr, s.uInt8Func);
    }
}

void Mapper0::deserialize(Deserializer& d) {
    d.deserializeVector(prgRam, d.uInt8Func);
    if (hasChrRam()) {
        d.deserializeVector(chr, d.uInt8Func);
    }
}