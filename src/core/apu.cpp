#include "core/apu.hpp"

#include "core/bus.hpp"

APU::APU(Bus& bus) : bus(bus) {
    resetAPU();
}

void APU::resetAPU() {
    for (int i = 0; i < 2; i++) {
        pulses[i].data = 0;
        pulses[i].i = {};
    }

    triangle.data = 0;
    triangle.i = {};

    noise.data = 0;
    noise.i = {};
    noise.i.shiftRegister = 1;

    dmc.data = 0;
    dmc.i = {};
    dmc.i.currentAddress = 0xC000;
    dmc.i.sampleBufferEmpty = true;
    dmc.i.silenceFlag = true;
    dmc.i.bitsRemaining = 8;

    status.data = 0;

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
                pulse.reg4000 = value;
                break;

            case 1: // 0x4001 / 0x4005
                pulse.reg4001 = value;

                pulse.i.sweepReloadFlag = true;

                if (!pulse.sweepUnitEnabled || !pulse.sweepUnitShift) {
                    pulse.i.sweepMutesChannel = false;
                }
                break;

            case 2: // 0x4002 / 0x4006
                pulse.reg4002 = value;
                break;

            default: // 0x4003 / 0x4007
                pulse.reg4003 = value;

                pulse.i.timerCounter = pulse.timer;

                if (getPulseStatus(pulseNum)) {
                    pulse.i.lengthCounter = LENGTH_COUNTER_TABLE[pulse.lengthCounterLoad];
                }

                pulse.i.envelopeStartFlag = true;
                pulse.i.dutyCycleIndex = 0;
                break;
        }
    }
    else if (TRIANGLE_RANGE.contains(addr)) {
        switch (addr & 0x3) {
            case 0: // 0x4008
                triangle.reg4008 = value;
                break;

            case 1: // 0x4009
                // Unused
                break;

            case 2: // 0x400A
                triangle.reg400A = value;
                break;

            default: // 0x400B
                triangle.reg400B = value;

                triangle.i.timerCounter = triangle.timer;

                if (status.enableTriangle) {
                    triangle.i.lengthCounter = LENGTH_COUNTER_TABLE[triangle.lengthCounterLoad];
                }
                triangle.i.linearCounterReloadFlag = true;
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
                noise.i.timerCounter = NOISE_PERIOD_TABLE[noise.noisePeriod];
                break;

            default: // 0x400F
                noise.lengthCounterLoad = (value >> 3) & 0x1F;
                if (status.enableNoise) {
                    noise.i.lengthCounter = LENGTH_COUNTER_TABLE[noise.lengthCounterLoad];
                }
                noise.i.envelopeStartFlag = true;
                break;
        }
    }
    else if (DMC_RANGE.contains(addr)) {
        switch (addr & 0x3) {
            case 0: // 0x4010
                dmc.reg4010 = value;

                if (!dmc.irqEnable) {
                    dmc.i.irqFlag = false;
                }

                dmc.i.timerCounter = DMC_RATE_TABLE[dmc.frequency];
                break;

            case 1: // 0x4011
                dmc.reg4011 = value;
                break;

            case 2: // 0x4012
                dmc.reg4012 = value;
                break;

            case 3: // 0x4013
                dmc.reg4013 = value;
                break;
        }
    }
}

uint8_t APU::viewStatus() const {
    Status tempStatus{ status.data };

    tempStatus.enablePulse1 &= pulses[0].i.lengthCounter > 0;
    tempStatus.enablePulse2 &= pulses[1].i.lengthCounter > 0;
    tempStatus.enableTriangle &= triangle.i.lengthCounter > 0 && triangle.i.linearCounter > 0;
    tempStatus.enableNoise &= noise.i.lengthCounter > 0;
    tempStatus.enableDmc &= dmc.i.bytesRemaining > 0;
    tempStatus.frameInterrupt |= frameInterruptFlag;
    tempStatus.dmcInterrupt |= dmc.i.irqFlag;

    return tempStatus.data;
}

uint8_t APU::readStatus() {
    frameInterruptFlag = false;
    dmc.i.irqFlag = false;
    return viewStatus();
}

void APU::writeStatus(uint8_t value) {
    status.data = value;

    if (!status.enablePulse1) pulses[0].i.lengthCounter = 0;
    if (!status.enablePulse2) pulses[1].i.lengthCounter = 0;

    if (!status.enableTriangle) {
        triangle.i.lengthCounter = 0;
        triangle.i.outputValue = 0;
    }

    if (!status.enableNoise) noise.i.lengthCounter = 0;

    if (!status.enableDmc) {
        dmc.i.bytesRemaining = 0;
    }
    else if (dmc.i.bytesRemaining == 0) {
        // Silent until first sample is loaded
        dmc.i.silenceFlag = true;

        restartDmcSample();
    }
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
                break;
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
            if (getPulseStatus(i)) {
                Pulse& pulse = pulses[i];

                if (pulse.i.timerCounter == 0) {
                    uint16_t timerReload = pulse.timer;
                    pulse.i.timerCounter = timerReload;
                    pulse.i.dutyCycleIndex = (pulse.i.dutyCycleIndex + 1) & 0x7;
                }
                else {
                    pulse.i.timerCounter--;
                }
            }
        }

        // Clock noise timer
        if (status.enableNoise) {
            if (noise.i.timerCounter == 0) {
                noise.i.timerCounter = NOISE_PERIOD_TABLE[noise.noisePeriod];

                // Clock LFSR
                uint8_t shift = noise.loopNoise ? 6 : 1;
                uint8_t feedback = (noise.i.shiftRegister & 1) ^ ((noise.i.shiftRegister >> shift) & 1);

                noise.i.shiftRegister >>= 1;
                noise.i.shiftRegister |= (feedback << 14);
            }
            else {
                noise.i.timerCounter--;
            }
        }
    }

    // Clock triangle timer
    uint16_t triangleTimerPeriod = triangle.timer;
    if (status.enableTriangle && triangleTimerPeriod >= 2) {
        if (triangle.i.lengthCounter > 0 && triangle.i.linearCounter > 0) {
            if (triangle.i.timerCounter == 0) {
                triangle.i.timerCounter = triangleTimerPeriod;
                triangle.i.sequenceIndex = (triangle.i.sequenceIndex + 1) & 0x1F;
                triangle.i.outputValue = TRIANGLE_SEQUENCE[triangle.i.sequenceIndex];
            }
            else {
                triangle.i.timerCounter--;
            }
        }
    }

    // Clock DMC reader
    if (status.enableDmc) {
        if (dmc.i.timerCounter == 0) {
            dmc.i.timerCounter = DMC_RATE_TABLE[dmc.frequency];

            if (!dmc.i.silenceFlag) {
                bool shiftBit = dmc.i.shiftRegister & 1;
                dmc.i.shiftRegister >>= 1;

                // Update output level
                if (shiftBit) {
                    if (dmc.outputLevel <= 125) {
                        dmc.outputLevel += 2;
                    }
                }
                else {
                    if (dmc.outputLevel >= 2) {
                        dmc.outputLevel -= 2;
                    }
                }

                dmc.i.bitsRemaining--;
                if (dmc.i.bitsRemaining == 0) {
                    dmc.i.bitsRemaining = 8;

                    if (dmc.i.sampleBufferEmpty) {
                        dmc.i.silenceFlag = true;
                    }
                    else {
                        dmc.i.silenceFlag = false;
                        dmc.i.shiftRegister = dmc.i.sampleBuffer;
                        dmc.i.sampleBufferEmpty = true;

                        // Reset bits counter when loading new sample
                        dmc.i.bitsRemaining = 8;

                        // Try to reload sample buffer via DMA
                        if (dmc.i.bytesRemaining) {
                            bus.requestDmcDma(dmc.i.currentAddress);
                        }
                        else if (dmc.i.bytesRemaining == 0) {
                            if (dmc.loopSample) {
                                restartDmcSample();
                            }
                            else if (dmc.irqEnable) {
                                dmc.i.irqFlag = true;
                            }
                        }
                    }
                }
            }
        }
        else {
            dmc.i.timerCounter--;
        }
    }

    frameCounter++;
    totalCycles++;
}

void APU::quarterClock() {
    // Clock pulse envelopes
    {
        for (int i = 0; i < 2; ++i) {
            Pulse& pulse = pulses[i];
            if (pulse.i.envelopeStartFlag) {
                pulse.i.envelopeStartFlag = false;
                pulse.i.envelope = 0xF;
                pulse.i.envelopeDividerCounter = pulse.volumeOrEnvelopeRate;
            }
            else {
                if (pulse.i.envelopeDividerCounter > 0) {
                    pulse.i.envelopeDividerCounter--;
                }
                else {
                    pulse.i.envelopeDividerCounter = pulse.volumeOrEnvelopeRate;
                    if (pulse.i.envelope) {
                        pulse.i.envelope--;
                    }
                    else if (pulse.envelopeLoopOrLengthCounterHalt) {
                        pulse.i.envelope = 0xF;
                    }
                }
            }
        }
    }

    // Clock noise envelope
    {
        if (noise.i.envelopeStartFlag) {
            noise.i.envelopeStartFlag = false;
            noise.i.envelope = 0xF;
            noise.i.envelopeDividerCounter = noise.volumeOrEnvelope;
        }
        else {
            if (noise.i.envelopeDividerCounter > 0) {
                noise.i.envelopeDividerCounter--;
            }
            else {
                noise.i.envelopeDividerCounter = noise.volumeOrEnvelope;
                if (noise.i.envelope) {
                    noise.i.envelope--;
                }
                else if (noise.envelopeLoopOrLengthCounterHalt) {
                    noise.i.envelope = 0xF;
                }
            }
        }
    }

    // Clock triangle linear counter
    {
        if (triangle.i.linearCounterReloadFlag) {
            triangle.i.linearCounter = triangle.linearCounterLoad;
        }
        else if (triangle.i.linearCounter > 0) {
            triangle.i.linearCounter--;
        }

        if (!triangle.lengthCounterHaltOrLinearCounterControl) {
            triangle.i.linearCounterReloadFlag = false;
        }
    }
}

void APU::halfClock() {
    // Clock length counters
    {
        // Pulse
        for (int i = 0; i < 2; i++) {
            if (getPulseStatus(i) && pulses[i].i.lengthCounter && !pulses[i].envelopeLoopOrLengthCounterHalt) pulses[i].i.lengthCounter--;
        }

        // Triangle
        if (status.enableTriangle && triangle.i.lengthCounter > 0 && !triangle.lengthCounterHaltOrLinearCounterControl) {
            triangle.i.lengthCounter--;
        }

        // Noise
        if (status.enableNoise && noise.i.lengthCounter > 0 && !noise.envelopeLoopOrLengthCounterHalt) {
            noise.i.lengthCounter--;
        }
    }

    // Clock sweep units
    {
        for (int i = 0; i < 2; i++) {
            Pulse& pulse = pulses[i];
            bool sweepClockedThisTick = false;

            if (pulse.i.sweepReloadFlag) {
                pulse.i.sweepDividerCounter = pulse.sweepUnitPeriod + 1;
                sweepClockedThisTick = true;
                pulse.i.sweepReloadFlag = false;
            }

            if (pulse.i.sweepDividerCounter) {
                pulse.i.sweepDividerCounter--;
            }

            if (!pulse.i.sweepDividerCounter) {
                sweepClockedThisTick = true;
                pulse.i.sweepDividerCounter = pulse.sweepUnitPeriod + 1;
            }

            if (sweepClockedThisTick) {
                if (pulse.sweepUnitEnabled && pulse.sweepUnitShift > 0 && pulse.i.lengthCounter > 0) {
                    uint16_t currentPeriod = pulse.timer;
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
                        pulse.i.sweepMutesChannel = true;
                    }
                    else {
                        pulse.i.sweepMutesChannel = false;
                        pulse.timer = targetPeriod;
                    }
                }
            }
        }
    }
}

bool APU::irqRequested() const {
    return frameInterruptFlag || dmc.i.irqFlag;
}

float APU::getAudioSample() const {
    // https://www.nesdev.org/wiki/APU_Mixer

    // The NES APU mixer takes the channel outputs and converts them to an analog audio signal. 
    // Each channel has its own internal digital-to-analog convertor (DAC), implemented in a way that causes non-linearity and interaction between channels, so calculation of the resulting amplitude is somewhat involved.
    // In particular, games such as Super Mario Bros. and StarTropics use the DMC level ($4011) as a crude volume control for the triangle and noise channels.

    // The following formula[1] calculates the approximate audio output level within the range of 0.0 to 1.0. It is the sum of two sub-groupings of the channels:

    // output = pulse_out + tnd_out

    //                             95.88
    // pulse_out = ------------------------------------
    //              (8128 / (pulse1 + pulse2)) + 100

    //                                        159.79
    // tnd_out = -------------------------------------------------------------
    //                                     1
    //            ----------------------------------------------------- + 100
    //             (triangle / 8227) + (noise / 12241) + (dmc / 22638)

    auto mixPulse = [](uint8_t pulse1, uint8_t pulse2) -> float {
        if (!pulse1 && !pulse2) return 0.0f;
        return 95.88f / ((8128.0f / (pulse1 + pulse2)) + 100.0f);
    };

    auto mixTND = [](uint8_t triangle, uint8_t noise, uint8_t dmc) -> float {
        if (!triangle && !noise && !dmc) return 0.0f;

        float sum = (triangle / 8227.0f) + (noise / 12241.0f) + (dmc / 22638.0f);
        return 159.79f / ((1.0f / sum) + 100.0f);
    };

    // Get pulse outputs
    std::array<uint8_t, 2> pulseOutputs = {};
    for (int i = 0; i < 2; i++) {
        const Pulse& pulse = pulses[i];
        if (getPulseStatus(i) && pulse.i.lengthCounter > 0 && pulse.i.timerCounter >= 9 && !pulse.i.sweepMutesChannel) {
            uint8_t dutyCycle = DUTY_CYCLES[pulse.duty];
            bool dutyOutput = (dutyCycle >> pulse.i.dutyCycleIndex) & 1;

            if (dutyOutput) {
                uint8_t volume = pulse.constantVolume ? pulse.volumeOrEnvelopeRate : pulse.i.envelope;
                pulseOutputs[i] = volume;
            }
        }
    }

    // Get triangle outputs
    uint8_t triangleOutput = triangle.i.outputValue;

    // Get noise output
    uint8_t noiseOutput = 0;
    bool shiftRegisterBitSet = noise.i.shiftRegister & 1;
    if (status.enableNoise && noise.i.lengthCounter > 0 && !shiftRegisterBitSet) {
        uint8_t volume = noise.constantVolume ? noise.volumeOrEnvelope : noise.i.envelope;
        noiseOutput = volume;
    }

    // Get DMC output
    uint8_t dmcOutput = dmc.outputLevel;

    return mixPulse(pulseOutputs[0], pulseOutputs[1]) + mixTND(triangleOutput, noiseOutput, dmcOutput);
}

void APU::receiveDMCSample(uint8_t sample) {
    dmc.i.sampleBuffer = sample;
    dmc.i.sampleBufferEmpty = false;
    dmc.i.silenceFlag = false;

    if (dmc.i.currentAddress == 0xFFFF) {
        dmc.i.currentAddress = 0x8000;
    }
    else {
        dmc.i.currentAddress++;
    }

    dmc.i.bytesRemaining--;
}

void APU::restartDmcSample() {
    dmc.i.currentAddress = 0xC000 | (dmc.sampleAddress << 6);
    dmc.i.bytesRemaining = (dmc.sampleLength << 4) + 1;

    // Request the first sample of the new loop
    bus.requestDmcDma(dmc.i.currentAddress);
}

bool APU::getPulseStatus(bool pulseNum) const {
    return !pulseNum ? status.enablePulse1 : status.enablePulse2;
}

void APU::serialize(Serializer& s) const {
    auto serializePulse = [](Serializer& s, const Pulse& pulse) -> void {
        s.serializeUInt32(pulse.data);

        // Internal state
        s.serializeUInt16(pulse.i.timerCounter);
        s.serializeUInt8(pulse.i.dutyCycleIndex);
        s.serializeUInt8(pulse.i.lengthCounter);
        s.serializeBool(pulse.i.envelopeStartFlag);
        s.serializeUInt8(pulse.i.envelope);
        s.serializeUInt8(pulse.i.envelopeDividerCounter);
        s.serializeUInt8(pulse.i.sweepDividerCounter);
        s.serializeBool(pulse.i.sweepMutesChannel);
        s.serializeBool(pulse.i.sweepReloadFlag);
    };

    auto serializeTriangle = [](Serializer& s, const Triangle& triangle) -> void {
        s.serializeUInt32(triangle.data);

        // Internal state
        s.serializeUInt16(triangle.i.timerCounter);
        s.serializeUInt8(triangle.i.linearCounter);
        s.serializeBool(triangle.i.linearCounterReloadFlag);
        s.serializeUInt8(triangle.i.sequenceIndex);
        s.serializeUInt8(triangle.i.lengthCounter);
        s.serializeUInt8(triangle.i.outputValue);
    };

    auto serializeNoise = [](Serializer& s, const Noise& noise) -> void {
        s.serializeUInt16(noise.data);

        // Internal state
        s.serializeUInt16(noise.i.timerCounter);
        s.serializeUInt8(noise.i.lengthCounter);
        s.serializeBool(noise.i.envelopeStartFlag);
        s.serializeUInt8(noise.i.envelope);
        s.serializeUInt8(noise.i.envelopeDividerCounter);
        s.serializeUInt16(noise.i.shiftRegister);
    };

    auto serializeDMC = [](Serializer& s, const DMC& dmc) -> void {
        s.serializeUInt32(dmc.data);

        // Internal state
        s.serializeUInt16(dmc.i.currentAddress);
        s.serializeUInt16(dmc.i.bytesRemaining);
        s.serializeUInt16(dmc.i.timerCounter);
        s.serializeUInt8(dmc.i.sampleBuffer);
        s.serializeBool(dmc.i.sampleBufferEmpty);
        s.serializeUInt8(dmc.i.shiftRegister);
        s.serializeUInt8(dmc.i.bitsRemaining);
        s.serializeBool(dmc.i.silenceFlag);
        s.serializeBool(dmc.i.irqFlag);
    };

    serializePulse(s, pulses[0]);
    serializePulse(s, pulses[1]);
    serializeTriangle(s, triangle);
    serializeNoise(s, noise);
    serializeDMC(s, dmc);

    s.serializeUInt8(status.data);
    s.serializeBool(frameSequenceMode);
    s.serializeBool(interruptInhibitFlag);
    s.serializeBool(frameInterruptFlag);
    s.serializeUInt64(frameCounter);
    s.serializeUInt64(totalCycles);
}

void APU::deserialize(Deserializer& d) {
    auto deserializePulse = [](Deserializer& d, Pulse& pulse) -> void {
        d.deserializeUInt32(pulse.data);

        // Internal state
        d.deserializeUInt16(pulse.i.timerCounter);
        d.deserializeUInt8(pulse.i.dutyCycleIndex);
        d.deserializeUInt8(pulse.i.lengthCounter);
        d.deserializeBool(pulse.i.envelopeStartFlag);
        d.deserializeUInt8(pulse.i.envelope);
        d.deserializeUInt8(pulse.i.envelopeDividerCounter);
        d.deserializeUInt8(pulse.i.sweepDividerCounter);
        d.deserializeBool(pulse.i.sweepMutesChannel);
        d.deserializeBool(pulse.i.sweepReloadFlag);
    };

    auto deserializeTriangle = [](Deserializer& d, Triangle& triangle) -> void {
        d.deserializeUInt32(triangle.data);

        // Internal state
        d.deserializeUInt16(triangle.i.timerCounter);
        d.deserializeUInt8(triangle.i.linearCounter);
        d.deserializeBool(triangle.i.linearCounterReloadFlag);
        d.deserializeUInt8(triangle.i.sequenceIndex);
        d.deserializeUInt8(triangle.i.lengthCounter);
        d.deserializeUInt8(triangle.i.outputValue);
    };

    auto deserializeNoise = [](Deserializer& d, Noise& noise) -> void {
        d.deserializeUInt16(noise.data);

        // Internal state
        d.deserializeUInt16(noise.i.timerCounter);
        d.deserializeUInt8(noise.i.lengthCounter);
        d.deserializeBool(noise.i.envelopeStartFlag);
        d.deserializeUInt8(noise.i.envelope);
        d.deserializeUInt8(noise.i.envelopeDividerCounter);
        d.deserializeUInt16(noise.i.shiftRegister);
    };

    auto deserializeDMC = [](Deserializer& d, DMC& dmc) -> void {
        d.deserializeUInt32(dmc.data);

        // Internal state
        d.deserializeUInt16(dmc.i.currentAddress);
        d.deserializeUInt16(dmc.i.bytesRemaining);
        d.deserializeUInt16(dmc.i.timerCounter);
        d.deserializeUInt8(dmc.i.sampleBuffer);
        d.deserializeBool(dmc.i.sampleBufferEmpty);
        d.deserializeUInt8(dmc.i.shiftRegister);
        d.deserializeUInt8(dmc.i.bitsRemaining);
        d.deserializeBool(dmc.i.silenceFlag);
        d.deserializeBool(dmc.i.irqFlag);
    };

    deserializePulse(d, pulses[0]);
    deserializePulse(d, pulses[1]);
    deserializeTriangle(d, triangle);
    deserializeNoise(d, noise);
    deserializeDMC(d, dmc);

    d.deserializeUInt8(status.data);
    d.deserializeBool(frameSequenceMode);
    d.deserializeBool(interruptInhibitFlag);
    d.deserializeBool(frameInterruptFlag);
    d.deserializeUInt64(frameCounter);
    d.deserializeUInt64(totalCycles);
}