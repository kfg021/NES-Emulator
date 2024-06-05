#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "cartridge.hpp"

#include "../util/util.hpp"

#include <stdint.h>
#include <array>
#include <memory>

class Bus{
public:
    static constexpr MemoryRange RAM_ADDRESSABLE_RANGE{0x0000, 0x1FFF};
    static constexpr MemoryRange PPU_ADDRESSABLE_RANGE{0x2000, 0x3FFF};
    static constexpr MemoryRange APU_ADDRESSABLE_RANGE{0x4000, 0x401F};
    static constexpr MemoryRange CARTRIDGE_ADDRESSABLE_RANGE{0x4020, 0xFFFF};

    Bus(const std::shared_ptr<Cartridge>& cartridge);
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

private:
    std::array<uint8_t, 0x800> ram;
    std::shared_ptr<Cartridge> cartridge;
};

#endif // MEMORY_HPP