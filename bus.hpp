#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <stdint.h>
#include <array>

class Bus{
public:
    static const int MEMORY_SIZE = 1 << 16;
    Bus();
    void clear();
    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);

private:
    std::array<uint8_t, MEMORY_SIZE> data;
};

#endif // MEMORY_HPP