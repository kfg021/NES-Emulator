#ifndef PPU_HPP
#define PPU_HPP

#include "bus.hpp"
#include "cartridge.hpp"

#include <memory>
#include <optional>

class Bus;

class PPU{
public:
    PPU();
    void setBus(Bus* bus);
    void setCartridge(std::shared_ptr<Cartridge>& cartridge);

    enum Register{
        CONTROL = 0x2000,
        MASK,
        STATUS,
        OAMADDR,
        OAMDATA,
        SCROLL,
        ADDR,
        DATA,
        OAMDMA = 0x4014
    };

    std::optional<uint8_t> read(uint16_t address);
    void write(uint16_t address, uint8_t value);

private:
    // Pointer to the Bus instance that the PPU is attached to. The CPU is not responsible for clearing this memory as it will get deleted when the Bus goes out of scope
    Bus* bus;

    std::shared_ptr<Cartridge> cartridge;
};

#endif // PPU_HPP