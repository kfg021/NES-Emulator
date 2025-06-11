#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <string>
#include <type_traits>

static constexpr uint32_t KB = (1 << 10);

template <uint32_t x>
constexpr uint32_t MASK() {
    constexpr bool isPowerOf2 = (x > 0) && !(x & (x - 1));
    static_assert(isPowerOf2, "Input to MASK must be power of 2");
    return x - 1;
}

template <uint8_t Offset, uint8_t Size, typename T = uint8_t>
class BitField {
    static_assert(std::is_unsigned<T>::value, "BitField type must be unsigned");
    static_assert(Size > 0, "BitField size must be positive");
    static_assert(Offset + Size <= 8 * sizeof(T), "Bit subset must be fit within BitField type");

public:
    BitField(T& data) : data(data) {}

    operator T() const { return get(); }

    BitField& operator=(T rhs) { set(rhs); return *this; }
    BitField& operator=(const BitField& rhs) { set(static_cast<T>(rhs)); return *this; }

    BitField& operator+=(T rhs) { set(get() + rhs); return *this; }
    BitField& operator-=(T rhs) { set(get() - rhs); return *this; }
    BitField& operator&=(T rhs) { set(get() & rhs); return *this; }
    BitField& operator|=(T rhs) { set(get() | rhs); return *this; }
    BitField& operator^=(T rhs) { set(get() ^ rhs); return *this; }

    BitField& operator<<=(uint8_t rhs) { set(get() << rhs); return *this; }
    BitField& operator>>=(uint8_t rhs) { set(get() >> rhs); return *this; }

    BitField& operator++() { set(get() + 1); return *this; }
    BitField& operator--() { set(get() - 1); return *this; }

private:
    static constexpr T unshiftedMask = (static_cast<T>(1) << Size) - 1;
    static constexpr T shiftedMask = unshiftedMask << Offset;

    T& data;

    T get() const {
        return (data >> Offset) & unshiftedMask;
    }

    void set(T value) {
        data = (data & ~shiftedMask) | ((value & unshiftedMask) << Offset);
    }
};

struct MemoryRange {
    const uint16_t lo, hi;
    constexpr int size() const { return hi - lo + 1; }
    constexpr bool contains(uint16_t addr) const { return (addr >= lo) && (addr <= hi); }
};

std::string toHexString8(uint8_t x);
std::string toHexString16(uint16_t x);
std::string toHexString32(uint32_t x);

#endif // UTIL_HPP