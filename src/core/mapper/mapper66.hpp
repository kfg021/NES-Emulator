#ifndef MAPPER66_HPP
#define MAPPER66_HPP

#include "mapper.hpp"

#include "../../util/util.hpp"

class Mapper66 : public Mapper {
public:
    Mapper66(uint8_t prgRomChunks, uint8_t chrRomChunks);

    std::optional<uint32_t> mapToPRGView(uint16_t cpuAddress) const override;
    std::optional<uint32_t> mapToPRGRead(uint16_t cpuAddress) override;
    std::optional<uint32_t> mapToPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    std::optional<uint32_t> mapToCHRView(uint16_t ppuAddress) const override;
    std::optional<uint32_t> mapToCHRRead(uint16_t ppuAddress) override;
    std::optional<uint32_t> mapToCHRWrite(uint16_t ppuAddress, uint8_t value) override;

private:
    static constexpr MemoryRange PRG_RANGE{ 0x8000, 0xFFFF };
    static constexpr MemoryRange CHR_RANGE{ 0x0000, 0x1FFF };
    static constexpr MemoryRange BANK_SELECT_RANGE{ 0x8000, 0xFFFF };
    uint8_t currentPRGBank;
    uint8_t currentCHRBank;
};

#endif // MAPPER66_HPP