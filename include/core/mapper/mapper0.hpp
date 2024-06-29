#ifndef MAPPER0_HPP
#define MAPPER0_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

class Mapper0 : public Mapper {
public:
    Mapper0(uint8_t prgRomChunks, uint8_t chrRomChunks, MirrorMode mirrorMode);

    std::optional<uint32_t> mapToPRGView(uint16_t cpuAddress) const override;
    std::optional<uint32_t> mapToPRGRead(uint16_t cpuAddress) override;
    std::optional<uint32_t> mapToPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    std::optional<uint32_t> mapToCHRView(uint16_t ppuAddress) const override;
    std::optional<uint32_t> mapToCHRRead(uint16_t ppuAddress) override;
    std::optional<uint32_t> mapToCHRWrite(uint16_t ppuAddress, uint8_t value) override;

private:
    static constexpr MemoryRange PRG_RANGE{ 0x8000, 0xFFFF };
    static constexpr MemoryRange CHR_RANGE{ 0x0000, 0x1FFF };
};

#endif // MAPPER0_HPP