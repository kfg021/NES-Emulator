#include "core/mapper/mapper.hpp"

#include "core/cartridge.hpp"
#include "core/mapper/mapper0.hpp"
#include "core/mapper/mapper1.hpp"
#include "core/mapper/mapper2.hpp"
#include "core/mapper/mapper3.hpp"
#include "core/mapper/mapper4.hpp"
#include "core/mapper/mapper66.hpp"

// TODO: Maybe it is worth refactoring to avoid having to copy prg and chr.
// However, since the Mapper constructor is only called once per ROM, it is not super critical
Mapper::Mapper(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : config(config), prg(prg), chr(chr) {

    if (config.hasBatteryBackedPrgRam) {
        // TODO: Load this from a save file
        prgRam = std::vector<uint8_t>(PRG_RAM_RANGE.size(), 0);
    }
}

std::unique_ptr<Mapper> Mapper::createMapper(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) {
    switch (config.id) {
        case 0:     return std::make_unique<Mapper0>(config, prg, chr);
        case 1:     return std::make_unique<Mapper1>(config, prg, chr);
        case 2:     return std::make_unique<Mapper2>(config, prg, chr);
        case 3:     return std::make_unique<Mapper3>(config, prg, chr);
        case 4:     return std::make_unique<Mapper4>(config, prg, chr);
        case 66:    return std::make_unique<Mapper66>(config, prg, chr);
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
    return config.hasBatteryBackedPrgRam && PRG_RAM_RANGE.contains(address);
}

uint8_t Mapper::getPrgRam(uint16_t address) const {
    return prgRam[address & 0x1FFF];
}

void Mapper::setPrgRam(uint16_t address, uint8_t value) {
    prgRam[address & 0x1FFF] = value;
}

Mapper::MirrorMode Mapper::getMirrorMode() const {
    return config.initialMirrorMode;
}