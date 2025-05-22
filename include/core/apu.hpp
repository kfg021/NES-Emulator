#ifndef APU_HPP
#define APU_HPP

#include "util/util.hpp"

#include <array>
#include <cstdint>

class APU {
public:
    APU();

    void initAPU();
    void write(uint16_t addr, uint8_t value);

    // Handles reads to/writes from 0x4015
    uint8_t viewStatus() const;
    uint8_t readStatus();
    void writeStatus(uint8_t value);

    // Handles writes to 0x4017
    void writeFrameCounter(uint8_t value);

    void executeHalfCycle();
    bool irqRequested() const;

    float getAudioSample() const;
private:
    constexpr static MemoryRange PULSE_RANGE{ 0x4000, 0x4007 };
    constexpr static MemoryRange TRIANGLE_RANGE{ 0x4008, 0x400B };
    constexpr static MemoryRange NOISE_RANGE{ 0x400C, 0x400F };
    constexpr static MemoryRange DMC_RANGE{ 0x4010, 0x4013 };

    struct Pulse {
        // 0x4000 / 0x4004
        uint8_t volumeOrEnvelopeRate : 4;
        uint8_t constantVolume : 1;
        uint8_t envelopeLoopOrLengthCounterHalt : 1;
        uint8_t duty : 2;

        // 0x4001 / 0x4005
        uint8_t sweepUnitShift : 3;
        bool sweepUnitNegate : 1;
        uint8_t sweepUnitPeriod : 3;
        bool sweepUnitEnabled : 1;

        // 0x4002 / 0x4006
        uint8_t timerLow;

        // 0x4003 / 0x4007
        uint8_t timerHigh : 3;
        uint8_t lengthCounterLoad : 5;

        // Internal state
        uint16_t timerCounter : 11;
        uint8_t dutyCycleIndex : 3;
        uint8_t lengthCounter;
        bool envelopeStartFlag;
        uint8_t envelope : 4;
        uint8_t envelopeDividerCounter;
        uint8_t sweepDividerCounter;
        bool sweepMutesChannel : 1;
        bool sweepReloadFlag : 1;
    };

    struct Triangle {
        // 0x4008
        uint8_t linearCounterLoad : 7;
        bool lengthCounterHaltOrLinearCounterControl : 1;

        // 0x400A
        uint8_t timerLow;

        // 0x400B
        uint8_t timerHigh : 3;
        uint8_t lengthCounterLoad : 5;

        // Internal state
        uint16_t timerCounter : 11;
        uint8_t linearCounter : 7;
        bool linearCounterReloadFlag : 1;
        uint8_t sequenceIndex : 5;
        uint8_t lengthCounter;
        uint8_t outputValue;
    };

    const std::array<uint8_t, 4> DUTY_CYCLES = {
        0b00000001,
        0b00000011,
        0b00001111,
        0b11111100
    };

    std::array<Pulse, 2> pulses;
    Triangle triangle;

    uint8_t status;

    bool frameSequenceMode;
    bool interruptInhibitFlag;
    bool frameInterruptFlag;

    uint64_t frameCounter;
    uint64_t totalCycles;

    constexpr static std::array<int, 5> STEP_SEQUENCE = { 7457, 14913, 22371, 29829, 37281 };
    constexpr static int FOUR_STEP_SEQUENCE_LENGTH = 29830;
    constexpr static int FIVE_STEP_SEQUENCE_LENGTH = 37282;

    constexpr static std::array<uint8_t, 0x20> LENGTH_COUNTER_TABLE = {
        10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
        12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
    };

    constexpr static std::array<uint8_t, 32> TRIANGLE_SEQUENCE = {
        15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    };

    void quarterClock();
    void halfClock();
};

#endif // APU_HPP