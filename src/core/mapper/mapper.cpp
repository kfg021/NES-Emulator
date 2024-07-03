#include "core/mapper/mapper.hpp"

#include "core/cartridge.hpp"
#include "core/mapper/mapper0.hpp"
#include "core/mapper/mapper1.hpp"
#include "core/mapper/mapper2.hpp"
#include "core/mapper/mapper3.hpp"
#include "core/mapper/mapper66.hpp"

// TODO: Maybe it is worth refactoring to avoid having to copy prg and chr.
// However, since the Mapper constructor is only called once per ROM, it is not super critical
Mapper::Mapper(uint8_t prgChunks, uint8_t chrChunks, MirrorMode initialMirrorMode, bool hasBatteryBackedPrgRam, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : prgChunks(prgChunks),
    chrChunks(chrChunks),
    initialMirrorMode(initialMirrorMode),
    hasBatteryBackedPrgRam(hasBatteryBackedPrgRam),
    prg(prg),
    chr(chr) {

    if (hasBatteryBackedPrgRam) {
        // TODO: Load this from a save file
        prgRam = std::vector<uint8_t>(PRG_RAM_RANGE.size(), 0);
    }
}

std::unique_ptr<Mapper> Mapper::createMapper(uint16_t id, uint8_t prgChunks, uint8_t chrChunks, bool mirrorModeId, bool hasBatteryBackedPrgRam, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) {
    MirrorMode mirrorMode = static_cast<MirrorMode>(mirrorModeId);
    switch (id) {
        case 0:     return std::make_unique<Mapper0>(prgChunks, chrChunks, mirrorMode, hasBatteryBackedPrgRam, prg, chr);
        case 1:     return std::make_unique<Mapper1>(prgChunks, chrChunks, mirrorMode, hasBatteryBackedPrgRam, prg, chr);
        case 2:     return std::make_unique<Mapper2>(prgChunks, chrChunks, mirrorMode, hasBatteryBackedPrgRam, prg, chr);
        case 3:     return std::make_unique<Mapper3>(prgChunks, chrChunks, mirrorMode, hasBatteryBackedPrgRam, prg, chr);
        case 66:    return std::make_unique<Mapper66>(prgChunks, chrChunks, mirrorMode, hasBatteryBackedPrgRam, prg, chr);
        default:    return nullptr; // TODO: Add more mappers
    }
}

uint8_t Mapper::mapPRGRead(uint16_t cpuAddress) {
    return mapPRGView(cpuAddress);
}

uint8_t Mapper::mapCHRRead(uint16_t ppuAddress) {
    return mapCHRView(ppuAddress);
}

bool Mapper::canAccessPrgRam(uint16_t address) const {
    return hasBatteryBackedPrgRam && PRG_RAM_RANGE.contains(address);
}

uint8_t Mapper::getPrgRam(uint16_t address) const {
    return prgRam[address & 0x1FFF];
}

void Mapper::setPrgRam(uint16_t address, uint8_t value) {
    prgRam[address & 0x1FFF] = value;
}

Mapper::MirrorMode Mapper::getMirrorMode() const {
    return initialMirrorMode;
}