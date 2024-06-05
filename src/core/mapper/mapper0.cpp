#include "mapper0.hpp"

#include "../../util/util.hpp"

Mapper0::Mapper0(uint8_t prgRomChunks, uint8_t chrRomChunks) 
    : Mapper(prgRomChunks, chrRomChunks){
}

static const MemoryRange PRG_RANGE{0x8000, 0xFFFF};
static const MemoryRange CHR_RANGE{0x0000, 0x1FFF};

std::optional<uint32_t> Mapper0::mapToPRGRead(uint16_t cpuAddress){
    if(PRG_RANGE.contains(cpuAddress)){
        if(prgRomChunks == 1){
            return cpuAddress & 0x3FFF;
        }
        else{
            // TODO: check that prgRomChunks == 2
            return cpuAddress & 0x7FFF;
        }
    }
    else{
        return std::nullopt;
    }
}
std::optional<uint32_t> Mapper0::mapToPRGWrite(uint16_t cpuAddress){
    // TODO: Figure out if mapper 0 should actually have PRG RAM
    return mapToPRGRead(cpuAddress);
}
std::optional<uint32_t> Mapper0::mapToCHRRead(uint16_t ppuAddress){
    if(CHR_RANGE.contains(ppuAddress)){
        return ppuAddress;
    }
    else{
        return std::nullopt;
    }
}
std::optional<uint32_t> Mapper0::mapToCHRWrite(uint16_t ppuAddress){
    if(CHR_RANGE.contains(ppuAddress) && chrRomChunks == 0){
        // If chrRomChunks == 0, we assume we have CHR RAM
        return ppuAddress;
    }
    else{
        return std::nullopt;
    }
}