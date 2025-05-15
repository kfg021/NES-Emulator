#ifndef GUITYPES_HPP
#define GUITYPES_HPP

#include "core/ppu.hpp"

#include <array>
#include <atomic>
#include <queue>

using ControllerStatus = std::array<std::atomic<uint8_t>, 2>;
struct KeyboardInput {
    const ControllerStatus* controllerStatus;
    std::atomic<bool>* resetFlag;

    // Debug window settings
    const std::atomic<bool>* debugWindowEnabled;
    const std::atomic<bool>* stepModeEnabled;
    const std::atomic<bool>* stepRequested;
    const std::atomic<uint8_t>* spritePallete;
    const std::atomic<uint8_t>* backgroundPallete;
};

struct DebugWindowState {
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint8_t sr;
    uint8_t backgroundPallete;
    uint8_t spritePallete;
    std::array<uint32_t, 0x20> palleteRamColors;
    PPU::PatternTable table1, table2;
    std::queue<uint16_t> recentPCs;
};

#endif // GUITYPES_HPP 