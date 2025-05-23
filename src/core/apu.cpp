#include "core/apu.hpp"

APU::APU() {
    initAPU();
}

void APU::initAPU() {
    pulses[0] = {};
    pulses[1] = {};
    triangle = {};

    noise = {};
    noise.shiftRegister = 1; // Shift register is 1 on power-up

    status = 0;
    frameCounter = 0;
    totalCycles = 0;

    frameSequenceMode = false;
    interruptInhibitFlag = false;
    frameInterruptFlag = false;
}

void APU::write(uint16_t addr, uint8_t value) {
    if (PULSE_RANGE.contains(addr)) {
        bool pulseNum = (addr >> 2) & 1;
        Pulse& pulse = pulses[pulseNum];

        switch (addr & 0x3) {
            case 0: // 0x4000 / 0x4004
                pulse.volumeOrEnvelopeRate = value & 0xF;
                pulse.constantVolume = (value >> 4) & 0x1;
                pulse.envelopeLoopOrLengthCounterHalt = (value >> 5) & 0x1;
                pulse.duty = (value >> 6) & 0x3;
                break;

            case 1: // 0x4001 / 0x4005
                pulse.sweepUnitShift = value & 0x7;
                pulse.sweepUnitNegate = (value >> 3) & 0x1;
                pulse.sweepUnitPeriod = (value >> 4) & 0x7;
                pulse.sweepUnitEnabled = (value >> 7) & 0x1;
                pulse.sweepReloadFlag = true;

                if (!pulse.sweepUnitEnabled || !pulse.sweepUnitShift) {
                    pulse.sweepMutesChannel = false;
                }
                break;

            case 2: // 0x4002 / 0x4006
                pulse.timerLow = value;
                break;

            default: // 0x4003 / 0x4007
                pulse.timerHigh = value & 0x7;
                uint16_t timerReload = (static_cast<uint16_t>(pulse.timerHigh) << 8) | pulse.timerLow;
                pulse.timerCounter = timerReload;

                pulse.lengthCounterLoad = (value >> 3) & 0x1F;
                bool pulseStatus = (status >> pulseNum) & 1;
                if (pulseStatus) {
                    pulse.lengthCounter = LENGTH_COUNTER_TABLE[pulse.lengthCounterLoad];
                }

                pulse.envelopeStartFlag = true;
                pulse.dutyCycleIndex = 0;
                break;
        }
    }
    else if (TRIANGLE_RANGE.contains(addr)) {
        switch (addr & 0x3) {
            case 0: // 0x4008
                triangle.linearCounterLoad = value & 0x7F;
                triangle.lengthCounterHaltOrLinearCounterControl = (value >> 7) & 1;
                break;
            case 1: // 0x4009
                // Unused
                break;
            case 2: // 0x400A
                triangle.timerLow = value;
                break;
            default: // 0x400B
                triangle.timerHigh = value & 0x7;
                uint16_t timerReload = (static_cast<uint16_t>(triangle.timerHigh) << 8) | triangle.timerLow;
                triangle.timerCounter = timerReload;

                triangle.lengthCounterLoad = (value >> 3) & 0x1F;
                bool triangleStatus = (status >> 2) & 1;
                if (triangleStatus) {
                    triangle.lengthCounter = LENGTH_COUNTER_TABLE[triangle.lengthCounterLoad];
                }
                triangle.linearCounterReloadFlag = true;
                break;
        }
    }
    else if (NOISE_RANGE.contains(addr)) {
        switch (addr & 0x3) {
            case 0: // 0x400C
                noise.volumeOrEnvelope = value & 0xF;
                noise.constantVolume = (value >> 4) & 0x1;
                noise.envelopeLoopOrLengthCounterHalt = (value >> 5) & 0x1;
                break;
            case 1: // 0x400D
                // Unused
                break;
            case 2: // 0x400E
                noise.noisePeriod = value & 0xF;
                noise.loopNoise = (value >> 7) & 0x1;
                noise.timerCounter = NOISE_PERIOD_TABLE[noise.noisePeriod];
                break;
            default: // 0x400F
                noise.lengthCounterLoad = (value >> 3) & 0x1F;
                bool noiseStatus = (status >> 3) & 1;
                if (noiseStatus) {
                    noise.lengthCounter = LENGTH_COUNTER_TABLE[noise.lengthCounterLoad];
                }
                noise.envelopeStartFlag = true;
                break;
        }
    }

    // TODO: DMC channel
}

uint8_t APU::viewStatus() const {
    uint8_t tempStatus = 0;

    for (int i = 0; i < 2; i++) {
        bool pulseStatus = (status >> i) & 1;
        if (pulseStatus && pulses[i].lengthCounter > 0) {
            tempStatus |= (1 << i);
        }
    }

    bool triangleStatus = (status >> 2) & 1;
    if (triangleStatus && triangle.lengthCounter > 0 && triangle.linearCounter > 0) {
        tempStatus |= (1 << 2);
    }

    bool noiseStatus = (status >> 3) & 1;
    if (noiseStatus && noise.lengthCounter > 0) {
        tempStatus |= (1 << 3);
    }

    if (frameInterruptFlag) {
        tempStatus |= (1 << 6);
    }

    // $4015 read	IF-D NT21	DMC interrupt (I), frame interrupt (F), DMC active (D), length counter > 0 (N/T/2/1)
    // TODO: I, D

    return tempStatus;
}

uint8_t APU::readStatus() {
    frameInterruptFlag = false;
    return viewStatus();
}

void APU::writeStatus(uint8_t value) {
    if (!((value >> 0) & 1)) pulses[0].lengthCounter = 0;
    if (!((value >> 1) & 1)) pulses[1].lengthCounter = 0;

    // Triangle channel
    if (!((value >> 2) & 1)) {
        triangle.lengthCounter = 0;
        triangle.outputValue = 0;
    }

    // Noise channel
    if (!((value >> 3) & 1)) {
        noise.lengthCounter = 0;
    }

    // TODO: DMC channel

    status = value;
}

void APU::writeFrameCounter(uint8_t value) {
    frameSequenceMode = (value >> 7) & 1;
    interruptInhibitFlag = (value >> 6) & 1;

    if (interruptInhibitFlag) {
        frameInterruptFlag = false;
    }

    if (!frameSequenceMode) {
        quarterClock();
        halfClock();
    }

    frameCounter = 0;
}

void APU::executeHalfCycle() {
    bool quarterClockCycle = false;
    bool halfClockCycle = false;

    if (!frameSequenceMode) {
        // 4 step sequence
        switch (frameCounter % FOUR_STEP_SEQUENCE_LENGTH) {
            case STEP_SEQUENCE[0]:
                quarterClockCycle = true;
                break;
            case STEP_SEQUENCE[1]:
                quarterClockCycle = true;
                halfClockCycle = true;
                break;
            case STEP_SEQUENCE[2]:
                quarterClockCycle = true;
                break;
            case STEP_SEQUENCE[3] - 1:
                frameInterruptFlag = !interruptInhibitFlag;
                break;
            case STEP_SEQUENCE[3]:
                quarterClockCycle = true;
                halfClockCycle = true;
                frameInterruptFlag = !interruptInhibitFlag;
                break;
            case 0: /*STEP_SEQUENCE[3] + 1*/
                if (frameCounter > 0) frameInterruptFlag = !interruptInhibitFlag;
            default:
                break;
        }
    }
    else {
        // 5 step sequence
        switch (frameCounter % FIVE_STEP_SEQUENCE_LENGTH) {
            case STEP_SEQUENCE[0]:
                quarterClockCycle = true;
                break;
            case STEP_SEQUENCE[1]:
                quarterClockCycle = true;
                halfClockCycle = true;
                break;
            case STEP_SEQUENCE[2]:
                quarterClockCycle = true;
                break;
            case STEP_SEQUENCE[3]:
                // Do nothing
                break;
            case STEP_SEQUENCE[4]:
                quarterClockCycle = true;
                halfClockCycle = true;
                frameInterruptFlag = !interruptInhibitFlag;
                break;
            default:
                break;
        }
    }

    if (quarterClockCycle) {
        quarterClock();
    }

    if (halfClockCycle) {
        halfClock();
    }

    if (totalCycles & 1) {
        // Clock pulse timers
        for (int i = 0; i < 2; i++) {
            bool pulseStatus = (status >> i) & 1;
            if (pulseStatus) {
                Pulse& pulse = pulses[i];

                if (pulse.timerCounter == 0) {
                    uint16_t timerReload = (static_cast<uint16_t>(pulse.timerHigh) << 8) | pulse.timerLow;
                    pulse.timerCounter = timerReload;
                    pulse.dutyCycleIndex++;
                }
                else {
                    pulse.timerCounter--;
                }
            }
        }

        // Clock noise timer
        bool noiseStatus = (status >> 3) & 1;
        if (noiseStatus) {
            if (noise.timerCounter == 0) {
                noise.timerCounter = NOISE_PERIOD_TABLE[noise.noisePeriod];

                // Clock LFSR
                uint8_t shift = noise.loopNoise ? 6 : 1;
                uint8_t feedback = (noise.shiftRegister & 1) ^ ((noise.shiftRegister >> shift) & 1);

                noise.shiftRegister >>= 1;
                noise.shiftRegister |= (feedback << 14);
            }
            else {
                noise.timerCounter--;
            }
        }
    }

    // Clock triangle timer
    bool triangleStatus = (status >> 2) & 1;
    uint16_t triangleTimerPeriod = (static_cast<uint16_t>(triangle.timerHigh) << 8) | triangle.timerLow;
    if (triangleStatus && triangleTimerPeriod >= 2) {
        if (triangle.lengthCounter > 0 && triangle.linearCounter > 0) {
            if (triangle.timerCounter == 0) {
                triangle.timerCounter = triangleTimerPeriod;
                triangle.sequenceIndex++;
                triangle.outputValue = TRIANGLE_SEQUENCE[triangle.sequenceIndex];
            }
            else {
                triangle.timerCounter--;
            }
        }
    }

    frameCounter++;
    totalCycles++;
}

void APU::quarterClock() {
    // Clock pulse envelopes
    {
        for (int i = 0; i < 2; ++i) {
            Pulse& p = pulses[i];
            if (p.envelopeStartFlag) {
                p.envelopeStartFlag = false;
                p.envelope = 0xF;
                p.envelopeDividerCounter = p.volumeOrEnvelopeRate;
            }
            else {
                if (p.envelopeDividerCounter > 0) {
                    p.envelopeDividerCounter--;
                }
                else {
                    p.envelopeDividerCounter = p.volumeOrEnvelopeRate;
                    if (p.envelope) {
                        p.envelope--;
                    }
                    else if (p.envelopeLoopOrLengthCounterHalt) {
                        p.envelope = 15;
                    }
                }
            }
        }
    }

    // Clock noise envelope
    {
        if (noise.envelopeStartFlag) {
            noise.envelopeStartFlag = false;
            noise.envelope = 0xF;
            noise.envelopeDividerCounter = noise.volumeOrEnvelope;
        }
        else {
            if (noise.envelopeDividerCounter > 0) {
                noise.envelopeDividerCounter--;
            }
            else {
                noise.envelopeDividerCounter = noise.volumeOrEnvelope;
                if (noise.envelope) {
                    noise.envelope--;
                }
                else if (noise.envelopeLoopOrLengthCounterHalt) {
                    noise.envelope = 0xF;
                }
            }
        }
    }

    // Clock triangle linear counter
    {
        if (triangle.linearCounterReloadFlag) {
            triangle.linearCounter = triangle.linearCounterLoad;
        }
        else if (triangle.linearCounter > 0) {
            triangle.linearCounter--;
        }

        if (!triangle.lengthCounterHaltOrLinearCounterControl) {
            triangle.linearCounterReloadFlag = false;
        }
    }
}

void APU::halfClock() {
    // Clock length counters
    {
        // Pulse
        for (int i = 0; i < 2; i++) {
            bool pulseStatus = (status >> i) & 1;
            if (pulseStatus && pulses[i].lengthCounter && !pulses[i].envelopeLoopOrLengthCounterHalt) pulses[i].lengthCounter--;
        }

        // Triangle
        bool triangleStatus = (status >> 2) & 1;
        if (triangleStatus && triangle.lengthCounter > 0 && !triangle.lengthCounterHaltOrLinearCounterControl) {
            triangle.lengthCounter--;
        }

        // Noise
        bool noiseStatus = (status >> 3) & 1;
        if (noiseStatus && noise.lengthCounter > 0 && !noise.envelopeLoopOrLengthCounterHalt) {
            noise.lengthCounter--;
        }
    }

    // Clock sweep units
    {
        for (int i = 0; i < 2; i++) {
            Pulse& pulse = pulses[i];
            bool sweepClockedThisTick = false;

            if (pulse.sweepReloadFlag) {
                pulse.sweepDividerCounter = pulse.sweepUnitPeriod + 1;
                sweepClockedThisTick = true;
                pulse.sweepReloadFlag = false;
            }

            if (pulse.sweepDividerCounter) {
                pulse.sweepDividerCounter--;
            }

            if (!pulse.sweepDividerCounter) {
                sweepClockedThisTick = true;
                pulse.sweepDividerCounter = pulse.sweepUnitPeriod + 1;
            }

            if (sweepClockedThisTick) {
                if (pulse.sweepUnitEnabled && pulse.sweepUnitShift > 0 && pulse.lengthCounter > 0) {
                    uint16_t currentPeriod = (static_cast<uint16_t>(pulse.timerHigh) << 8) | pulse.timerLow;
                    uint16_t changeAmount = currentPeriod >> pulse.sweepUnitShift;
                    uint16_t targetPeriod;

                    if (pulse.sweepUnitNegate) {
                        targetPeriod = currentPeriod - changeAmount;

                        // Pulse 1 and pulse 2 are treated differently
                        if (i == 0) {
                            targetPeriod--;
                        }
                    }
                    else {
                        targetPeriod = currentPeriod + changeAmount;
                    }

                    if (currentPeriod < 8 || targetPeriod > 0x7FF) {
                        pulse.sweepMutesChannel = true;
                    }
                    else {
                        pulse.sweepMutesChannel = false;
                        pulse.timerLow = targetPeriod & 0xFF;
                        pulse.timerHigh = (targetPeriod >> 8) & 0x07;
                    }
                }
            }
        }
    }
}

bool APU::irqRequested() const {
    return frameInterruptFlag;
}

float APU::getAudioSample() const {
    auto mixPulse = [](uint8_t pulse1, uint8_t pulse2) -> float {
        if (pulse1 + pulse2 == 0) return 0.0f;
        return 95.88f / ((8128.0f / (pulse1 + pulse2)) + 100.0f);
    };

    auto mixTND = [](uint8_t triangle, uint8_t noise, uint8_t dmc) -> float {
        if (triangle + noise + dmc == 0) return 0.0f;

        float sum = (triangle / 8227.0f) + (noise / 12241.0f) + (dmc / 22638.0f);
        return 159.79f / ((1.0f / sum) + 100.0f);
    };

    // Get pulse outputs
    std::array<uint8_t, 2> pulseOutputs = {};
    for (int i = 0; i < 2; i++) {
        const Pulse& pulse = pulses[i];
        bool pulseStatus = (status >> i) & 1;

        if (pulseStatus && pulse.lengthCounter > 0 && pulse.timerCounter >= 9 && !pulse.sweepMutesChannel) {
            uint8_t dutyCycle = DUTY_CYCLES[pulse.duty];
            bool dutyOutput = (dutyCycle >> pulse.dutyCycleIndex) & 1;

            if (dutyOutput) {
                uint8_t volume = pulse.constantVolume ? pulse.volumeOrEnvelopeRate : pulse.envelope;
                pulseOutputs[i] = volume;
            }
        }
    }

    // Get triangle outputs
    uint8_t triangleOutput = triangle.outputValue;

    // Get noise output
    uint8_t noiseOutput = 0;
    bool noiseStatus = (status >> 3) & 1;
    bool shiftRegisterBitSet = noise.shiftRegister & 1;
    if (noiseStatus && noise.lengthCounter > 0 && !shiftRegisterBitSet) {
        uint8_t volume = noise.constantVolume ? noise.volumeOrEnvelope : noise.envelope;
        noiseOutput = volume;
    }

    // TODO: Mix DMC channel

    return mixPulse(pulseOutputs[0], pulseOutputs[1]) + mixTND(triangleOutput, noiseOutput, 0);
}
