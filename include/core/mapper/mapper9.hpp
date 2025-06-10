#ifndef MAPPER9_HPP
#define MAPPER9_HPP

#include "core/mapper/mapper.hpp"

#include <array>

class Mapper9 : public Mapper {
public:
    Mapper9(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    void reset() override;
    
    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    uint8_t mapCHRRead(uint16_t ppuAddress) override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

    MirrorMode getMirrorMode() const override;

    // Serialization
    void serialize(Serializer& s) const override;
    void deserialize(Deserializer& d) override;

private:
    // Banks
    static constexpr MemoryRange PRG_ROM_SWITCHABLE{ 0x8000, 0x9FFF };
    static constexpr MemoryRange PRG_ROM_FIXED{ 0xA000, 0xFFFF };
    static constexpr MemoryRange CHR_ROM_SWITCHABLE_1{ 0x0000, 0x0FFF };
    static constexpr MemoryRange CHR_ROM_SWITCHABLE_2{ 0x1000, 0x1FFF };

    // Registers
    static constexpr MemoryRange PRG_ROM_BANK_SELECT{ 0xA000, 0xAFFF };
    static constexpr MemoryRange CHR_ROM_BANK_1_SELECT_OPTION_1{ 0xB000, 0xBFFF };
    static constexpr MemoryRange CHR_ROM_BANK_1_SELECT_OPTION_2{ 0xC000, 0xCFFF };
    static constexpr MemoryRange CHR_ROM_BANK_2_SELECT_OPTION_1{ 0xD000, 0xDFFF };
    static constexpr MemoryRange CHR_ROM_BANK_2_SELECT_OPTION_2{ 0xE000, 0xEFFF };
    static constexpr MemoryRange MIRRORING{ 0xF000, 0xFFFF };

    static constexpr uint16_t LATCH_1_DISABLE = 0x0FD8;
    static constexpr uint16_t LATCH_1_ENABLE = 0x0FE8;
    static constexpr MemoryRange LATCH_2_DISABLE{ 0x1FD8, 0x1FDF };
    static constexpr MemoryRange LATCH_2_ENABLE{ 0x1FE8, 0x1FEF };

    uint8_t prgBankSelect;

    bool chrLatch1;
    bool chrLatch2;
    std::array<uint8_t, 2> chrBank1Select;
    std::array<uint8_t, 2> chrBank2Select;

    bool mirroring;

    PrgRam prgRam;
};

#endif // MAPPER9_HPP