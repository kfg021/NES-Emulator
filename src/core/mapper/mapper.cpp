#include "core/mapper/mapper.hpp"

#include "core/mapper/mapper0.hpp"
#include "core/mapper/mapper2.hpp"
#include "core/mapper/mapper3.hpp"
#include "core/mapper/mapper66.hpp"

Mapper::Mapper(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode)
    : prgRomChunks(prgRomChunks), chrRomChunks(chrRomChunks), mirrorMode(mirrorMode) {
}

std::unique_ptr<Mapper> Mapper::createMapper(uint8_t id, uint8_t prgRomChunks, uint8_t chrRomChunks, bool mirrorModeId) {
    MirrorMode mirrorMode = static_cast<MirrorMode>(mirrorModeId);
    switch (id) {
        case 0:     return std::make_unique<Mapper0>(prgRomChunks, chrRomChunks, mirrorMode);
        case 2:     return std::make_unique<Mapper2>(prgRomChunks, chrRomChunks, mirrorMode);
        case 3:     return std::make_unique<Mapper3>(prgRomChunks, chrRomChunks, mirrorMode);
        case 66:    return std::make_unique<Mapper66>(prgRomChunks, chrRomChunks, mirrorMode);
        default:    return nullptr; // TODO: Add more mappers
    }
}

Mapper::MirrorMode Mapper::getMirrorMode() const {
    return mirrorMode;
}