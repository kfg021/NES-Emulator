#include "core/mapper/mapper2.hpp"

#include "core/cartridge.hpp"

Mapper2::Mapper2(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(config, prg, chr),
    prgRam(config.hasBatteryBackedPrgRam),
    chrRam(config.chrChunks == 0) {

    reset();
}

void Mapper2::reset() {
    currentBank = 0;
}

uint8_t Mapper2::mapPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE_SWICHABLE.contains(cpuAddress)) {
        uint32_t mappedAddress = PRG_ROM_CHUNK_SIZE * currentBank + (cpuAddress & MASK<PRG_ROM_CHUNK_SIZE>());
        return prg[mappedAddress];
    }
    else if (PRG_RANGE_FIXED.contains(cpuAddress)) {
        uint32_t mappedAddress = PRG_ROM_CHUNK_SIZE * (config.prgChunks - 1) + (cpuAddress & MASK<PRG_ROM_CHUNK_SIZE>());
        return prg[mappedAddress];
    }
    else {
        return prgRam.tryRead(cpuAddress).value_or(0);
    }
}

void Mapper2::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value & 0x7;
    }
    else {
        prgRam.tryWrite(cpuAddress, value);
    }
}

uint8_t Mapper2::mapCHRView(uint16_t ppuAddress) const {
    return readChrRomOrRam(ppuAddress, chr, chrRam);
}

void Mapper2::mapCHRWrite(uint16_t ppuAddress, uint8_t value) {
    chrRam.tryWrite(ppuAddress, value);
}

void Mapper2::serialize(Serializer& s) const {
    s.serializeUInt8(currentBank);
    s.serializeVector(prgRam.data, s.uInt8Func);
    if (chrRam.isEnabled) {
        s.serializeVector(chrRam.data, s.uInt8Func);
    }
}

void Mapper2::deserialize(Deserializer& d) {
    d.deserializeUInt8(currentBank);
    d.deserializeVector(prgRam.data, d.uInt8Func);
    if (chrRam.isEnabled) {
        d.deserializeVector(chrRam.data, d.uInt8Func);
    }
}