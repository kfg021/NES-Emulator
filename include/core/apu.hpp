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
        uint8_t volumeEnvelope : 4;
        uint8_t constantVolume : 1;
        uint8_t lengthCounter : 1;
        uint8_t duty : 2;

        // 0x4001 / 0x4005
        uint8_t shift : 3;
        uint8_t negate : 1;
        uint8_t period : 3;
        uint8_t enabled : 1;

        // 0x4002 / 0x4006
        uint8_t timerLow;

        // 0x4003 / 0x4007
        uint8_t timerHigh : 3;
        uint8_t lengthCounterLoad : 5;

        // Internal state
        uint16_t timerCounter : 11;
        uint8_t dutyCycleIndex : 3;
    };

    const std::array<uint8_t, 4> DUTY_CYCLES = {
        0b00000010,
        0b00000110,
        0b00011110,
        0b11111001
    };

    std::array<Pulse, 2> pulses;
    uint8_t status;
    uint8_t frameCounter;
};

#endif // APU_HPP