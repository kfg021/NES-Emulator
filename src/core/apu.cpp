#include "core/apu.hpp"

APU::APU() {
    pulses[0] = {};
    pulses[1] = {};

    status = 0;
    frameCounter = 0;
}

void APU::write(uint16_t addr, uint8_t value) {
    if (PULSE_RANGE.contains(addr)) {
        bool pulseNum = (addr >> 3) & 1;
        Pulse& pulse = pulses[pulseNum];

        switch (addr & 0x3) {
            case 0:
                pulse.volumeEnvelope = value & 0xF;
                pulse.constantVolume = (value >> 4) & 0x1;
                pulse.lengthCounter = (value >> 5) & 0x1;
                pulse.duty = (value >> 6) & 0x3;
                break;

            case 1:
                pulse.shift = value & 0x7;
                pulse.negate = (value >> 3) & 0x1;
                pulse.period = (value >> 4) & 0x7;
                pulse.enabled = (value >> 7) & 0x1;
                break;

            case 2:
                pulse.timerLow = value;
                break;

            default: /*case 3:*/
                pulse.timerHigh = value & 0x7;
                pulse.lengthCounterLoad = (value >> 3) & 0x1F;
                break;
        }
    }
}


uint8_t APU::viewStatus() {
    return status;
}

uint8_t APU::readStatus() {
    return status;
}

void APU::writeStatus(uint8_t value) {
    status = value;
}

void APU::writeFrameCounter(uint8_t value) {
    frameCounter = value;
}

void APU::executeCycle() {
    uint8_t output = 0x00;

    for (int i = 0; i < 2; i++) {
        if ((status >> i) & 1) {
            Pulse& pulse = pulses[i];
            pulse.timerCounter--;

            if (pulse.timerCounter == 0x7FF) {
                uint16_t timerReload = (pulse.timerHigh << 8) | pulse.timerLow;
                pulse.timerCounter = timerReload;

                if (timerReload >= 7) {
                    pulse.dutyCycleIndex++;
                }
            }
        }
    }
}

float APU::getAudioSample() {
    float output = 0.0f;
    for (int i = 0; i < 2; i++) {
        if ((status >> i) & 1) {
            Pulse& pulse = pulses[i];
            uint8_t dutyCycle = DUTY_CYCLES[pulse.duty];
            bool val = (dutyCycle >> pulse.dutyCycleIndex) & 1;
            output += val;
        }
    }

    return output / 10.0f;
}
