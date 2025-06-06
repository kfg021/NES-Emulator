#include "core/mapper/mapper7.hpp"

Mapper7::Mapper7(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : Mapper(config, prg, chr) {
    reset();
}

void Mapper7::reset() {
    bankSelect = 0;
}

uint8_t Mapper7::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        uint8_t currentBank = bankSelect & 0x7;
        uint32_t mappedAddress = (PRG_ROM_CHUNK_SIZE << 1) * currentBank + (cpuAddress & MASK<32 * KB>());
        return prg[mappedAddress];
    }
    else if (canAccessPrgRam(cpuAddress)) {
        return getPrgRam(cpuAddress);
    }
}

void Mapper7::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (PRG_RANGE.contains(cpuAddress)) {
        bankSelect = value;
    }
    else if (canAccessPrgRam(cpuAddress)) {
        setPrgRam(cpuAddress, value);
    }
}

uint8_t Mapper7::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[ppuAddress];
    }

    return 0;
}

void Mapper7::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    if (CHR_RANGE.contains(ppuAddress) && hasChrRam()) {
        chr[ppuAddress] = value;
    }
}

bool Mapper7::hasChrRam() const {
    // If chrChunks == 0, we assume we have CHR RAM
    return config.chrChunks == 0;
}

Mapper::MirrorMode Mapper7::getMirrorMode() const {
    bool mirrorMode = (bankSelect >> 4) & 1;
    return mirrorMode ? MirrorMode::ONE_SCREEN_UPPER_BANK : MirrorMode::ONE_SCREEN_LOWER_BANK;
}

void Mapper7::serialize(Serializer& s) const {
    s.serializeUInt8(bankSelect);
    s.serializeVector(prgRam, s.uInt8Func);
    if (hasChrRam()) {
        s.serializeVector(chr, s.uInt8Func);
    }
}

void Mapper7::deserialize(Deserializer& d) {
    d.deserializeUInt8(bankSelect);
    d.deserializeVector(prgRam, d.uInt8Func);
    if (hasChrRam()) {
        d.deserializeVector(chr, d.uInt8Func);
    }
}