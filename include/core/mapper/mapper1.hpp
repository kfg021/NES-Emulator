#ifndef MAPPER1_HPP
#define MAPPER1_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

#include <array>

class Mapper1 : public Mapper {
public:
    Mapper1(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    void reset() override;
    
    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

    MirrorMode getMirrorMode() const override;

    bool hasChrRam() const;

    // Serialization
    void serialize(Serializer& s) const override;
    void deserialize(Deserializer& d) override;

private:
    // Banks
    static constexpr MemoryRange PRG_ROM_BANK_0{ 0x8000, 0xBFFF };
    static constexpr MemoryRange PRG_ROM_BANK_1{ 0xC000, 0xFFFF };

    static constexpr MemoryRange CHR_ROM_BANK_0{ 0x0000, 0x0FFF };
    static constexpr MemoryRange CHR_ROM_BANK_1{ 0x1000, 0x1FFF };

    // External register
    static constexpr MemoryRange LOAD_REGISTER{ 0x8000, 0xFFFF };

    // Internal registers
    static constexpr MemoryRange CONTROL_REGISTER{ 0x8000, 0x9FFF };
    static constexpr MemoryRange CHR_REGISTER_0{ 0xA000, 0xBFFF };
    static constexpr MemoryRange CHR_REGISTER_1{ 0xC000, 0xDFFF };
    static constexpr MemoryRange PRG_REGISTER{ 0xE000, 0xFFFF };

    void internalRegisterWrite(uint16_t address, uint8_t value);

    uint8_t shiftRegister;
    static constexpr uint8_t SHIFT_REGISTER_RESET = 0x10;

    // Mapper 1 Control (https://www.nesdev.org/wiki/MMC1)
    // 4bit0
    // -----
    // CPPMM
    // |||||
    // |||++- Mirroring (0: one-screen, lower bank; 1: one-screen, upper bank;
    // |||               2: vertical; 3: horizontal)
    // |++--- PRG ROM bank mode (0, 1: switch 32 KB at $8000, ignoring low bit of bank number;
    // |                         2: fix first bank at $8000 and switch 16 KB bank at $C000;
    // |                         3: fix last bank at $C000 and switch 16 KB bank at $8000)
    // +----- CHR ROM bank mode (0: switch 8 KB at a time; 1: switch two separate 4 KB banks)
    struct Control {
        uint8_t mirroring : 2;
        uint8_t prgRomMode : 2;
        bool chrRomMode : 1;

        Control() = default;
        Control(uint8_t data);
        uint8_t toUInt8() const;
        void setFromUInt8(uint8_t data);
    };
    Control control;

    uint8_t chrBank0;
    uint8_t chrBank1;

    // 4bit0
    // -----
    // RPPPP
    // |||||
    // |++++- Select 16 KB PRG ROM bank (low bit ignored in 32 KB mode)
    // +----- MMC1B and later: PRG RAM chip enable (0: enabled; 1: disabled; ignored on MMC1A)
    //     MMC1A: Bit 3 bypasses fixed bank logic in 16K mode (0: fixed bank affects A17-A14;
    //     1: fixed bank affects A16-A14 and bit 3 directly controls A17)
    struct PRGBank {
        uint8_t prgRomSelect : 4;
        bool prgRamDisable : 1;

        PRGBank() = default;
        PRGBank(uint8_t data);
        uint8_t toUInt8() const;
        void setFromUInt8(uint8_t data);
    };
    PRGBank prgBank;
};

#endif // MAPPER1_HPP