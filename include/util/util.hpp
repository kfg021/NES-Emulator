#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <string>

static constexpr uint32_t KB = (1 << 10);

template <uint32_t x>
constexpr uint32_t MASK() {
    constexpr bool isPowerOf2 = (x > 0) && !(x & (x - 1));
    static_assert(isPowerOf2, "Input to MASK must be power of 2");
    return x - 1;
}

struct MemoryRange {
    const uint16_t lo, hi;
    constexpr int size() const { return hi - lo + 1; }
    constexpr bool contains(uint16_t addr) const { return (addr >= lo) && (addr <= hi); }
};

std::string toHexString8(uint8_t x);
std::string toHexString16(uint16_t x);
std::string toHexString32(uint32_t x);

#endif // UTIL_HPP