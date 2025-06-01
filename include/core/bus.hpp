#ifndef BUS_HPP
#define BUS_HPP

#include "core/apu.hpp"
#include "core/cartridge.hpp"
#include "core/controller.hpp"
#include "core/cpu.hpp"
#include "core/ppu.hpp"
#include "util/serializer.hpp"
#include "util/util.hpp"

#include <array>
#include <cstdint>

class Bus {
public:
    Bus();

    Cartridge::Status loadROM(const std::string& filePath);
    void initDevices();

    uint8_t view(uint16_t address) const;
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    // Devices connected to bus
    std::unique_ptr<CPU> cpu;
    std::unique_ptr<PPU> ppu;
    std::unique_ptr<APU> apu;

    uint64_t totalCycles;

    void executeCycle();

    void setController(bool controller, uint8_t value);

    void requestDmcDma(uint16_t address);

    struct OamDma {
        bool requested;
        bool ongoing;
        uint8_t page;
        uint8_t offset;
        uint8_t data;
    };

    struct DmcDma {
        bool requested;
        bool ongoing;
        uint16_t address;
        uint8_t data;
        uint8_t delay;
    };

    // Serialization
    void serialize(Serializer& s) const;
    void deserialize(Deserializer& d);

private:
    // Memory ranges for devices
    static constexpr MemoryRange RAM_ADDRESSABLE_RANGE{ 0x0000, 0x1FFF };
    static constexpr MemoryRange PPU_ADDRESSABLE_RANGE{ 0x2000, 0x3FFF };
    static constexpr MemoryRange IO_ADDRESSABLE_RANGE{ 0x4000, 0x401F };
    static constexpr MemoryRange CARTRIDGE_ADDRESSABLE_RANGE{ 0x4020, 0xFFFF };

    std::array<uint8_t, 0x800> ram;

    static constexpr uint16_t CONTROLLER_1_DATA = 0x4016;
    static constexpr uint16_t CONTROLLER_2_DATA = 0x4017;

    // TODO: Consider adding NES zapper support
    std::array<Controller, 2> controllers;

    // After a write to 0x4016 we read controller information into here
    std::array<uint8_t, 2> controllerData;
    bool strobe;

    static constexpr uint16_t OAM_DMA_ADDR = 0x4014;

    OamDma oamDma;
    void oamDmaCycle();

    DmcDma dmcDma;
    void dmcDmaCycle();

    static constexpr MemoryRange APU_ADDRESSABLE_RANGE{ 0x4000, 0x4013 };
    static constexpr uint16_t APU_STATUS = 0x4015;
    static constexpr uint16_t APU_FRAME_COUNTER = 0x4017;
};

#endif // BUS_HPP