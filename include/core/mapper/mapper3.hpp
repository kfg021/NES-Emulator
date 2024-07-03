#ifndef MAPPER3_HPP
#define MAPPER3_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

class Mapper3 : public Mapper {
public:
    Mapper3(uint8_t prgChunks, uint8_t chrChunks, MirrorMode initialMirrorMode, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

private:
    static constexpr MemoryRange PRG_RANGE{ 0x8000, 0xFFFF };
    static constexpr MemoryRange CHR_RANGE{ 0x0000, 0x1FFF };
    static constexpr MemoryRange BANK_SELECT_RANGE{ 0x8000, 0xFFFF };
    uint8_t currentBank;
};

#endif // MAPPER3_HPP