#include "mapper.hpp"

#include "mapper0.hpp"
#include "mapper2.hpp"
#include "mapper3.hpp"

Mapper::Mapper(uint8_t prgRomChunks, uint8_t chrRomChunks) :
    prgRomChunks(prgRomChunks),
    chrRomChunks(chrRomChunks) {
}

std::unique_ptr<Mapper> Mapper::createMapper(uint8_t id, uint8_t prgRomChunks, uint8_t chrRomChunks) {
    switch (id) {
        case 0:     return std::make_unique<Mapper0>(prgRomChunks, chrRomChunks);
        case 2:     return std::make_unique<Mapper2>(prgRomChunks, chrRomChunks);
        case 3:     return std::make_unique<Mapper3>(prgRomChunks, chrRomChunks);
        default:    return nullptr; // TODO: Add more mappers
    }
}