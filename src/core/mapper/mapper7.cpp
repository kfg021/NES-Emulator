#include "core/mapper/mapper7.hpp"

Mapper7::Mapper7(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(config, prg, chr),
    prgRam(config.hasBatteryBackedPrgRam),
    chrRam(config.chrChunks == 0) {

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
    else {
        return prgRam.tryRead(cpuAddress).value_or(0);
    }
}

void Mapper7::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (PRG_RANGE.contains(cpuAddress)) {
        bankSelect = value;
    }
    else {
        prgRam.tryWrite(cpuAddress, value);
    }
}

uint8_t Mapper7::mapCHRView(uint16_t ppuAddress) const {
    return readChrRomOrRam(ppuAddress, chr, chrRam);
}

void Mapper7::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    chrRam.tryWrite(ppuAddress, value);
}

Mapper::MirrorMode Mapper7::getMirrorMode() const {
    bool mirrorMode = (bankSelect >> 4) & 1;
    return mirrorMode ? MirrorMode::ONE_SCREEN_UPPER_BANK : MirrorMode::ONE_SCREEN_LOWER_BANK;
}

void Mapper7::serialize(Serializer& s) const {
    s.serializeUInt8(bankSelect);
    s.serializeVector(prgRam.data, s.uInt8Func);
    if (chrRam.isEnabled) {
        s.serializeVector(chrRam.data, s.uInt8Func);
    }
}

void Mapper7::deserialize(Deserializer& d) {
    d.deserializeUInt8(bankSelect);
    d.deserializeVector(prgRam.data, d.uInt8Func);
    if (chrRam.isEnabled) {
        d.deserializeVector(chrRam.data, d.uInt8Func);
    }
}