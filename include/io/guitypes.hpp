#ifndef GUITYPES_HPP
#define GUITYPES_HPP

#include "core/ppu.hpp"
#include "io/threadsafeaudioqueue.hpp"

#include <array>
#include <memory>
#include <mutex>

#include <QMetaType>
#include <QString>

struct KeyboardInput {
    // NES controllers
    uint8_t controller1ButtonMask;
    uint8_t controller2ButtonMask;

    // Menu settings
    bool paused;
    bool muted;
    bool debugWindowEnabled;

    // Debug window settings
    uint8_t spritePallete;
    uint8_t backgroundPallete;

    // One-shot requests
    // The main thread will increment these variables each time there is a new request.
    // The emulator will use the (unsigned) difference between the current request count and its last seen request count to determine the number of requests to make.
    // Overflow is possible but will only be an issue if a key is somehow pressed over 255 times in a single frame :)
    uint8_t resetCount;
    uint8_t stepCount;
    uint8_t saveCount;
    uint8_t loadCount;

    // Data corresponding to a save/load request
    QString mostRecentSaveFilePath;
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
    std::shared_ptr<PPU::PatternTables> patternTables;
    std::array<QString, NUM_INSTS_TOTAL> insts;
};
Q_DECLARE_METATYPE(DebugWindowState)

static constexpr int AUDIO_SAMPLE_RATE = 44100;
static constexpr size_t AUDIO_QUEUE_MAX_CAPACITY = AUDIO_SAMPLE_RATE / 10; // Enough space to store 100ms of audio
using AudioQueue = ThreadSafeAudioQueue<AUDIO_QUEUE_MAX_CAPACITY>;

#endif // GUITYPES_HPP