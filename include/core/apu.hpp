#ifndef APU_HPP
#define APU_HPP

#include "util/serializer.hpp"
#include "util/util.hpp"

#include <array>
#include <cstdint>

class Bus;

class APU {
public:
    APU(Bus& bus);

    void resetAPU();

    void write(uint16_t addr, uint8_t value);

    // Handles reads to/writes from 0x4015
    uint8_t viewStatus() const;
    uint8_t readStatus();
    void writeStatus(uint8_t value);

    // Handles writes to 0x4017
    void writeFrameCounter(uint8_t value);

    void executeHalfCycle();
    bool irqRequested() const;

    void receiveDMCSample(uint8_t sample);

    float getAudioSample() const;

    // Serialization
    void serialize(Serializer& s) const;
    void deserialize(Deserializer& d);

private:
    static constexpr MemoryRange PULSE_RANGE{ 0x4000, 0x4007 };
    static constexpr MemoryRange TRIANGLE_RANGE{ 0x4008, 0x400B };
    static constexpr MemoryRange NOISE_RANGE{ 0x400C, 0x400F };
    static constexpr MemoryRange DMC_RANGE{ 0x4010, 0x4013 };

    struct Pulse {
        uint32_t data;
        BitField<0, 8, uint32_t>  reg4000{ data };
        BitField<8, 8, uint32_t>  reg4001{ data };
        BitField<16, 8, uint32_t> reg4002{ data };
        BitField<24, 8, uint32_t> reg4003{ data };

        // 0x4000 / 0x4004
        BitField<0, 4, uint32_t> volumeOrEnvelopeRate{ data };
        BitField<4, 1, uint32_t> constantVolume{ data };
        BitField<5, 1, uint32_t> envelopeLoopOrLengthCounterHalt{ data };
        BitField<6, 2, uint32_t> duty{ data };

        // 0x4001 / 0x4005  
        BitField<8, 3, uint32_t> sweepUnitShift{ data };
        BitField<11, 1, uint32_t> sweepUnitNegate{ data };
        BitField<12, 3, uint32_t> sweepUnitPeriod{ data };
        BitField<15, 1, uint32_t> sweepUnitEnabled{ data };

        // 0x4002 / 0x4006
        // 0x4003 / 0x4007
        BitField<16, 11, uint32_t> timer{ data }; // Use one combined timer instead of timer low/high
        BitField<27, 5, uint32_t> lengthCounterLoad{ data };

        struct Internal {
            uint16_t timerCounter;
            uint8_t dutyCycleIndex;
            uint8_t lengthCounter;
            bool envelopeStartFlag;
            uint8_t envelope;
            uint8_t envelopeDividerCounter;
            uint8_t sweepDividerCounter;
            bool sweepMutesChannel;
            bool sweepReloadFlag;
        };

        Internal i;
    };

    struct Triangle {
        uint32_t data;
        BitField<0, 8, uint32_t>  reg4008{ data };
        BitField<16, 8, uint32_t> reg400A{ data };
        BitField<24, 8, uint32_t> reg400B{ data };

        // 0x4008
        BitField<0, 7, uint32_t> linearCounterLoad{ data };
        BitField<7, 1, uint32_t> lengthCounterHaltOrLinearCounterControl{ data };

        // 0x400A
        // 0x400B
        BitField<16, 11, uint32_t> timer{ data }; // Use one combined timer instead of timer low/high
        BitField<27, 5, uint32_t>  lengthCounterLoad{ data };

        // Internal state
        struct Internal {
            uint16_t timerCounter;
            uint8_t linearCounter;
            bool linearCounterReloadFlag;
            uint8_t sequenceIndex;
            uint8_t lengthCounter;
            uint8_t outputValue;
        };

        Internal i;
    };

    struct Noise {
        // 0x400C
        uint8_t volumeOrEnvelope : 4;
        bool constantVolume : 1;
        bool envelopeLoopOrLengthCounterHalt : 1;

        // 0x400E
        uint8_t noisePeriod : 4;
        bool loopNoise : 1;

        // 0x400F
        uint8_t lengthCounterLoad : 5;

        // Internal state
        uint16_t timerCounter;
        uint8_t lengthCounter;
        bool envelopeStartFlag;
        uint8_t envelope;
        uint8_t envelopeDividerCounter;
        uint16_t shiftRegister;
    };

    struct DMC {
        // 0x4010
        uint8_t frequency : 4;
        bool loopSample : 1;
        bool irqEnable : 1;

        // 0x4011
        uint8_t outputLevel : 7;

        // 0x4012
        uint8_t sampleAddress;

        // 0x4013
        uint8_t sampleLength;

        // Internal state
        uint16_t currentAddress;
        uint16_t bytesRemaining;
        uint16_t timerCounter;
        uint8_t sampleBuffer;
        bool sampleBufferEmpty;
        uint8_t shiftRegister;
        uint8_t bitsRemaining;
        bool silenceFlag;
        bool irqFlag;
    };

    const std::array<uint8_t, 4> DUTY_CYCLES = {
        0b00000001,
        0b00000011,
        0b00001111,
        0b11111100
    };

    std::array<Pulse, 2> pulses;
    Triangle triangle;
    Noise noise;
    DMC dmc;

    uint8_t status;

    bool frameSequenceMode;
    bool interruptInhibitFlag;
    bool frameInterruptFlag;

    uint64_t frameCounter;
    uint64_t totalCycles;

    Bus& bus;

    static constexpr std::array<int, 5> STEP_SEQUENCE = { 7457, 14913, 22371, 29829, 37281 };
    static constexpr int FOUR_STEP_SEQUENCE_LENGTH = 29830;
    static constexpr int FIVE_STEP_SEQUENCE_LENGTH = 37282;

    static constexpr std::array<uint8_t, 0x20> LENGTH_COUNTER_TABLE = {
        10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
        12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
    };

    static constexpr std::array<uint8_t, 0x20> TRIANGLE_SEQUENCE = {
        15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    };

    static constexpr std::array<uint16_t, 0x10> NOISE_PERIOD_TABLE = {
        4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
    };

    static constexpr std::array<uint16_t, 0x10> DMC_RATE_TABLE = {
        428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
    };

    void quarterClock();
    void halfClock();

    void restartDmcSample();
};

#endif // APU_HPP