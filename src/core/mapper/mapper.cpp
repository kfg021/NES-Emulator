#include "core/mapper/mapper.hpp"

#include "core/mapper/mapper0.hpp"
#include "core/mapper/mapper2.hpp"
#include "core/mapper/mapper3.hpp"
#include "core/mapper/mapper66.hpp"

Mapper::Mapper(uint8_t prgRomChunks, uint8_t chrRomChunks)
    : prgRomChunks(prgRomChunks), chrRomChunks(chrRomChunks) {
}

std::unique_ptr<Mapper> Mapper::createMapper(uint8_t id, uint8_t prgRomChunks, uint8_t chrRomChunks) {
    switch (id) {
        case 0:     return std::make_unique<Mapper0>(prgRomChunks, chrRomChunks);
        case 2:     return std::make_unique<Mapper2>(prgRomChunks, chrRomChunks);
        case 3:     return std::make_unique<Mapper3>(prgRomChunks, chrRomChunks);
        case 66:    return std::make_unique<Mapper66>(prgRomChunks, chrRomChunks);
        default:    return nullptr; // TODO: Add more mappers
    }
}