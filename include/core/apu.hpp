#ifndef APU_HPP
#define APU_HPP

#include <cstdint>

class APU {
public:
    APU();

    void write(uint16_t addr, uint8_t value);

    // Handles reads to/writes from 0x4015
    uint8_t viewStatus();
    uint8_t readStatus();
    void writeStatus(uint8_t value);

    // Handles writes to 0x4017
    void writeFrameCounter(uint8_t value);

    float getAudioSample();
private:
};

#endif // APU_HPP