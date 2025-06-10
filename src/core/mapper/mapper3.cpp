#include "core/mapper/mapper3.hpp"

#include "core/cartridge.hpp"

Mapper3::Mapper3(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) :
    Mapper(config, prg, chr),
    prgRam(config.hasBatteryBackedPrgRam) {

    reset();
}

void Mapper3::reset() {
    currentBank = 0;
}

uint8_t Mapper3::mapPRGView(uint16_t cpuAddress) const {
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

void Mapper3::mapPRGWrite(uint16_t cpuAddress, uint8_t value) {
    if (BANK_SELECT_RANGE.contains(cpuAddress)) {
        currentBank = value;
    }
    else {
        prgRam.tryWrite(cpuAddress, value);
    }
}

uint8_t Mapper3::mapCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return chr[CHR_ROM_CHUNK_SIZE * currentBank + ppuAddress];
    }

    return 0;
}

void Mapper3::mapCHRWrite(uint16_t /*ppuAddress*/, uint8_t /*value*/) {
    // CHR in mapper 3 is read only
}

void Mapper3::serialize(Serializer& s) const {
    s.serializeUInt8(currentBank);
    s.serializeVector(prgRam.data, s.uInt8Func);
}

void Mapper3::deserialize(Deserializer& d) {
    d.deserializeUInt8(currentBank);
    d.deserializeVector(prgRam.data, d.uInt8Func);
}