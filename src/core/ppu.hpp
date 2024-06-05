#ifndef PPU_HPP
#define PPU_HPP

#include "bus.hpp"

#include <memory>

class Bus;

class PPU{
public:
    PPU();
    void setBus(const std::shared_ptr<Bus>& bus);

private:
    std::shared_ptr<Bus> bus;
};

#endif // PPU_HPP