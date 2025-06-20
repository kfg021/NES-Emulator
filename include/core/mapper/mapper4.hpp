#ifndef MAPPER4_HPP
#define MAPPER4_HPP

#include "core/mapper/mapper.hpp"

#include <array>

class Mapper4 : public Mapper {
public:
    Mapper4(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    void reset() override;

    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

    MirrorMode getMirrorMode() const override;

    void clockIRQTimer();
    bool irqRequested() const;

    // Serialization
    void serialize(Serializer& s) const override;
    void deserialize(Deserializer& d) override;

private:
    // PRG banks
    static constexpr std::array<MemoryRange, 2> PRG_ROM_8KB_SWITCHABLE_1{ {{0x8000, 0x9FFF}, {0xC000, 0xDFFF}} };
    static constexpr MemoryRange PRG_ROM_8KB_SWITCHABLE_2{ 0xA000, 0xBFFF };
    static constexpr std::array<MemoryRange, 2> PRG_ROM_8KB_FIXED_1{ {{0xC000, 0xDFFF}, {0x8000, 0x9FFF}} };
    static constexpr MemoryRange PRG_ROM_8KB_FIXED_2{ 0xE000, 0xFFFF };

    // CHR banks
    static constexpr std::array<MemoryRange, 2> CHR_ROM_2KB_SWITCHABLE_1{ {{0x0000, 0x07FF}, {0x1000, 0x17FF}} };
    static constexpr std::array<MemoryRange, 2> CHR_ROM_2KB_SWITCHABLE_2{ {{0x0800, 0x0FFF}, {0x1800, 0x1FFF}} };
    static constexpr std::array<MemoryRange, 2> CHR_ROM_1KB_SWITCHABLE_1{ {{0x1000, 0x13FF}, {0x0000, 0x03FF}} };
    static constexpr std::array<MemoryRange, 2> CHR_ROM_1KB_SWITCHABLE_2{ {{0x1400, 0x17FF}, {0x0400, 0x07FF}} };
    static constexpr std::array<MemoryRange, 2> CHR_ROM_1KB_SWITCHABLE_3{ {{0x1800, 0x1BFF}, {0x0800, 0x0BFF}} };
    static constexpr std::array<MemoryRange, 2> CHR_ROM_1KB_SWITCHABLE_4{ {{0x1C00, 0x1FFF}, {0x0C00, 0x0FFF}} };

    // Registers
    static constexpr MemoryRange BANK_SELECT_OR_BANK_DATA{ 0x8000, 0x9FFF };
    static constexpr MemoryRange MIRRORING_OR_PRG_RAM_PROTECT{ 0xA000, 0xBFFF };
    static constexpr MemoryRange IRQ_LATCH_OR_IRQ_RELOAD{ 0xC000, 0xDFFF };
    static constexpr MemoryRange IRQ_DISABLE_OR_IRQ_ENABLE{ 0xE000, 0xFFFF };

    // Some games use special nametable mirroring and have custom nametables within the mapper itself
    static constexpr MemoryRange ALTERNATIVE_NAMETABLE_RANGE{0x2000, 0x2FFF};
    std::vector<uint8_t> customNametable;

    uint8_t bankSelect;
    uint8_t bankData;
    bool mirroring;
    uint8_t prgRamProtect;
    uint8_t irqReloadValue;
    uint8_t irqTimer;
    bool irqEnabled;
    bool irqReloadPending;
    bool irqRequest;

    std::array<uint8_t, 2> prgSwitchableBankSelect;
    std::array<uint8_t, 6> chrSwitchableBankSelect;

    PrgRam prgRam;

    bool canReadFromPRGRam() const;
    bool canWriteToPRGRam() const;
};

#endif // MAPPER4_HPP