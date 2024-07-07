#ifndef MAPPER66_HPP
#define MAPPER66_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

class Mapper66 : public Mapper {
public:
    Mapper66(uint8_t prgChunks, uint8_t chrChunks, MirrorMode initialMirrorMode, bool hasBatteryBackedPrgRam, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

private:
    static constexpr MemoryRange BANK_SELECT_RANGE = PRG_RANGE;
    uint8_t currentPRGBank;
    uint8_t currentCHRBank;
};

#endif // MAPPER66_HPP