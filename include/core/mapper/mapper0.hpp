#ifndef MAPPER0_HPP
#define MAPPER0_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

class Mapper0 : public Mapper {
public:
    Mapper0(uint8_t prgChunks, uint8_t chrChunks, MirrorMode initialMirrorMode, bool hasBatteryBackedPrgRam, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;
};

#endif // MAPPER0_HPP