#ifndef GUITYPES_HPP
#define GUITYPES_HPP

#include "core/ppu.hpp"

#include <array>
#include <atomic>
#include <queue>

#include <QString>

using ControllerStatus = std::array<std::atomic<uint8_t>, 2>;
struct KeyboardInput {
    // Game input
    const ControllerStatus* controllerStatus;
    std::atomic<bool>* resetFlag;

    // Debug window settings
    const std::atomic<bool>* debugWindowEnabled;
    const std::atomic<uint8_t>* pauseFlag; // Treated as a boolean
    std::atomic<bool>* stepRequested;
    const std::atomic<uint8_t>* spritePallete;
    const std::atomic<uint8_t>* backgroundPallete;

    // Sound settings
    const std::atomic<uint8_t>* globalMuteFlag; // Treated as a boolean
};

struct DebugWindowState {
    static constexpr int NUM_INSTS_ABOVE_AND_BELOW = 9;
    static constexpr int NUM_INSTS_TOTAL = 2 * NUM_INSTS_ABOVE_AND_BELOW + 1;

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
    std::array<QString, NUM_INSTS_TOTAL> insts;
};

static constexpr int AUDIO_SAMPLE_RATE = 44100;	
static constexpr size_t AUDIO_QUEUE_MAX_CAPACITY = AUDIO_SAMPLE_RATE / 10;

#endif // GUITYPES_HPP 