#include "mapper.hpp"
#include "mapper0.hpp"

Mapper::Mapper(uint8_t prgRomChunks, uint8_t chrRomChunks) : 
    prgRomChunks(prgRomChunks),
    chrRomChunks(chrRomChunks){
}

std::unique_ptr<Mapper> Mapper::createMapper(uint8_t id, uint8_t prgRomChunks, uint8_t chrRomChunks){
    if(id == 0){
        return std::make_unique<Mapper0>(prgRomChunks, chrRomChunks);
    }
    else{
        // TODO: add more mappers
        return nullptr;
    }
}