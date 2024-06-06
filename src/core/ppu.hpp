#ifndef PPU_HPP
#define PPU_HPP

#include "bus.hpp"

#include <memory>

class Bus;

class PPU{
public:
    PPU();
    void setBus(Bus* bus);

private:
    Bus* bus;
};

#endif // PPU_HPP