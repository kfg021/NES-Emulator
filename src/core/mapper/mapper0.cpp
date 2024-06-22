#include "mapper0.hpp"

#include "../../util/util.hpp"

Mapper0::Mapper0(uint8_t prgRomChunks, uint8_t chrRomChunks)
    : Mapper(prgRomChunks, chrRomChunks) {
}

static constexpr MemoryRange PRG_RANGE{ 0x8000, 0xFFFF };
static constexpr MemoryRange CHR_RANGE{ 0x0000, 0x1FFF };

std::optional<uint32_t> Mapper0::mapToPRGView(uint16_t cpuAddress) const {
    if (PRG_RANGE.contains(cpuAddress)) {
        if (prgRomChunks == 1) {
            return cpuAddress & 0x3FFF;
        }
        else if (prgRomChunks == 2) {
            return cpuAddress & 0x7FFF;
        }
        else {
            return std::nullopt;
        }
    }
    else {
        return std::nullopt;
    }
}

std::optional<uint32_t> Mapper0::mapToPRGRead(uint16_t cpuAddress) {
    return mapToPRGView(cpuAddress);
}

std::optional<uint32_t> Mapper0::mapToPRGWrite(uint16_t /*cpuAddress*/) {
    // PRG in mapper 0 is read only
    return std::nullopt;
}


std::optional<uint32_t> Mapper0::mapToCHRView(uint16_t ppuAddress) const {
    if (CHR_RANGE.contains(ppuAddress)) {
        return ppuAddress;
    }
    else {
        return std::nullopt;
    }
}

std::optional<uint32_t> Mapper0::mapToCHRRead(uint16_t ppuAddress) {
    return mapToCHRView(ppuAddress);
}

std::optional<uint32_t> Mapper0::mapToCHRWrite(uint16_t ppuAddress) {
    if (CHR_RANGE.contains(ppuAddress) && chrRomChunks == 0) {
        // If chrRomChunks == 0, we assume we have CHR RAM
        return ppuAddress;
    }
    else {
        return std::nullopt;
    }
}