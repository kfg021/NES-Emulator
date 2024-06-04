#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <stdint.h>
#include <array>

struct MemoryRange{
    const uint16_t lo, hi;
    constexpr int size() const{ return hi - lo + 1; }
    constexpr bool contains(uint16_t addr) const{ return addr >= lo && addr <= hi; }
};

class Bus{
public:
    // static constexpr MemoryRange RAM_RANGE{0x1000, 0x1FFF};
    static constexpr MemoryRange RAM_RANGE{0x0000, 0xFFFF};

    Bus();
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

private:
    std::array<uint8_t, RAM_RANGE.size()> ram;
};

#endif // MEMORY_HPP