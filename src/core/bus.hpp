#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "cartridge.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "../util/util.hpp"

#include <stdint.h>
#include <array>
#include <memory>

class CPU;
class PPU;

class Bus{
public:
    Bus();

    bool loadROM(const std::string& filePath);
    void initDevices();

    uint8_t view(uint16_t address) const;
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    std::unique_ptr<CPU> cpu;
    std::unique_ptr<PPU> ppu;

    uint64_t totalCycles;
    bool nmiRequest;

    void executeCycle();

private:
    static constexpr MemoryRange RAM_ADDRESSABLE_RANGE{0x0000, 0x1FFF};
    static constexpr MemoryRange PPU_ADDRESSABLE_RANGE{0x2000, 0x3FFF};
    static constexpr MemoryRange APU_ADDRESSABLE_RANGE{0x4000, 0x401F};
    static constexpr MemoryRange CARTRIDGE_ADDRESSABLE_RANGE{0x4020, 0xFFFF};

    std::array<uint8_t, 0x800> ram;
    std::shared_ptr<Cartridge> cartridge;
};

#endif // MEMORY_HPP