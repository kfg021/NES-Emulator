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
    uint8_t viewStatus();
    uint8_t readStatus();
    void writeStatus(uint8_t value);

    // Handles writes to 0x4017
    void writeFrameCounter(uint8_t value);

    void executeCycle();

    float getAudioSample();
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
        uint8_t sweepUnitNegate : 1;
        uint8_t sweepUnitPeriod : 3;
        uint8_t sweepUnitEnabled : 1;

        // 0x4002 / 0x4006
        uint8_t timerLow;

        // 0x4003 / 0x4007
        uint8_t timerHigh : 3;
        uint8_t lengthCounterLoad : 5;

        // Internal state
        uint16_t timerCounter : 11;
        uint8_t dutyCycleIndex : 3;
        uint8_t envelope : 4;
        uint8_t lengthCounter : 5;
    };

    const std::array<uint8_t, 4> DUTY_CYCLES = {
        0b00000010,
        0b00000110,
        0b00011110,
        0b11111001
    };

    std::array<Pulse, 2> pulses;
    uint8_t status;

    bool frameSequenceMode;
    bool interruptInhibitFlag;
    bool frameInterruptFlag;

    uint64_t frameCounter;
    uint64_t totalCycles;

    constexpr static std::array<int, 5> STEP_SEQUENCE = { 7457, 14913, 22371, 29829, 37281 };
    constexpr static int FOUR_STEP_SEQUENCE_LENGTH = 29830;
    constexpr static int FIVE_STEP_SEQUENCE_LENGTH = 37282;
};

#endif // APU_HPP