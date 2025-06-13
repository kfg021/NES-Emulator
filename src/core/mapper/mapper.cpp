#include "core/mapper/mapper.hpp"

#include "core/cartridge.hpp"
#include "core/mapper/mapper0.hpp"
#include "core/mapper/mapper1.hpp"
#include "core/mapper/mapper2.hpp"
#include "core/mapper/mapper3.hpp"
#include "core/mapper/mapper4.hpp"
#include "core/mapper/mapper7.hpp"
#include "core/mapper/mapper9.hpp"
#include "core/mapper/mapper66.hpp"

// TODO: Maybe it is worth refactoring to avoid having to copy prg and chr.
// However, since the Mapper constructor is only called once per ROM, it is not super critical
Mapper::Mapper(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr)
    : config(config), prg(prg), chr(chr) {
}

std::unique_ptr<Mapper> Mapper::createMapper(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) {
    switch (config.id) {
        case 0:     return std::make_unique<Mapper0>(config, prg, chr);
        case 1:     return std::make_unique<Mapper1>(config, prg, chr);
        case 2:     return std::make_unique<Mapper2>(config, prg, chr);
        case 3:     return std::make_unique<Mapper3>(config, prg, chr);
        case 4:     return std::make_unique<Mapper4>(config, prg, chr);
        case 7:     return std::make_unique<Mapper7>(config, prg, chr);
        case 9:     return std::make_unique<Mapper9>(config, prg, chr);
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

Mapper::MirrorMode Mapper::getMirrorMode() const {
    return config.initialMirrorMode;
}

uint8_t Mapper::readChrRomOrRam(uint32_t mappedAddress, const std::vector<uint8_t>& chr, const ChrRam& chrRam) {
    if (chrRam.isEnabled) {
        return chrRam.tryRead(static_cast<uint16_t>(mappedAddress)).value_or(0);
    }
    else {
        return chr[mappedAddress];
    }
}