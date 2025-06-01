#include "core/apu.hpp"

#include "core/bus.hpp"

APU::APU() {
    initAPU();
}

void APU::initAPU() {
    pulses[0] = {};
    pulses[1] = {};
    triangle = {};

    noise = {};
    noise.shiftRegister = 1; // Shift register is 1 on power-up

    dmc = {};
    dmc.currentAddress = 0xC000;
    dmc.bytesRemaining = 0;
    dmc.sampleBufferEmpty = true;
    dmc.silenceFlag = true;
    dmc.bitsRemaining = 8;
    dmc.irqFlag = false;
    dmc.shiftRegister = 0;

    status = 0;
    frameCounter = 0;
    totalCycles = 0;

    frameSequenceMode = false;
    interruptInhibitFlag = false;
    frameInterruptFlag = false;
}

void APU::setBus(Bus* bus) {
    this->bus = bus;
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
    else if (DMC_RANGE.contains(addr)) {
        switch (addr & 0x3) {
            case 0: // 0x4010
                dmc.frequency = value & 0xF;
                dmc.loopSample = (value >> 6) & 0x1;
                dmc.irqEnable = (value >> 7) & 0x1;

                if (!dmc.irqEnable) {
                    dmc.irqFlag = false;
                }

                dmc.timerCounter = DMC_RATE_TABLE[dmc.frequency];
                break;

            case 1: // 0x4011
                // Direct load to output level - bit 7 is ignored on NTSC
                dmc.outputLevel = value & 0x7F;
                break;

            case 2: // 0x4012
                dmc.sampleAddress = value;
                break;

            case 3: // 0x4013
                dmc.sampleLength = value;
                break;
        }
    }
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

    bool dmcStatus = (status >> 4) & 1;
    if (dmcStatus && dmc.bytesRemaining > 0) {
        tempStatus |= (1 << 4);
    }

    if (frameInterruptFlag) {
        tempStatus |= (1 << 6);
    }

    // DMC interrupt flag
    if (dmc.irqFlag) {
        tempStatus |= (1 << 7);
    }

    return tempStatus;
}

uint8_t APU::readStatus() {
    frameInterruptFlag = false;
    dmc.irqFlag = false;
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

    // DMC channel
    bool dmcStatus = (value >> 4) & 1;
    if (!dmcStatus) {
        dmc.bytesRemaining = 0;
    }
    else if (dmc.bytesRemaining == 0) {
        // Silent until first sample is loaded
        dmc.silenceFlag = true;

        restartDmcSample();
    }

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

    // Clock DMC reader
    bool dmcStatus = (status >> 4) & 1;
    if (dmcStatus) {
        if (dmc.timerCounter == 0) {
            dmc.timerCounter = DMC_RATE_TABLE[dmc.frequency];

            if (!dmc.silenceFlag) {
                bool shiftBit = dmc.shiftRegister & 1;
                dmc.shiftRegister >>= 1;

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

                dmc.bitsRemaining--;
                if (dmc.bitsRemaining == 0) {
                    dmc.bitsRemaining = 8;

                    if (dmc.sampleBufferEmpty) {
                        dmc.silenceFlag = true;
                    }
                    else {
                        dmc.silenceFlag = false;
                        dmc.shiftRegister = dmc.sampleBuffer;
                        dmc.sampleBufferEmpty = true;

                        // Reset bits counter when loading new sample
                        dmc.bitsRemaining = 8;

                        // Try to reload sample buffer via DMA
                        if (dmc.bytesRemaining) {
                            bus->requestDmcDma(dmc.currentAddress);
                        }
                        else if (dmc.bytesRemaining == 0) {
                            if (dmc.loopSample) {
                                restartDmcSample();
                            }
                            else if (dmc.irqEnable) {
                                dmc.irqFlag = true;
                            }
                        }
                    }
                }
            }
        }
        else {
            dmc.timerCounter--;
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
    return frameInterruptFlag || dmc.irqFlag;
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

    // Get DMC output
    uint8_t dmcOutput = dmc.outputLevel;

    return mixPulse(pulseOutputs[0], pulseOutputs[1]) + mixTND(triangleOutput, noiseOutput, dmcOutput);
}

void APU::receiveDMCSample(uint8_t sample) {
    dmc.sampleBuffer = sample;
    dmc.sampleBufferEmpty = false;
    dmc.silenceFlag = false;

    if (dmc.currentAddress == 0xFFFF) {
        dmc.currentAddress = 0x8000;
    }
    else {
        dmc.currentAddress++;
    }

    dmc.bytesRemaining--;
}

void APU::restartDmcSample() {
    dmc.currentAddress = 0xC000 | (static_cast<uint16_t>(dmc.sampleAddress) << 6);
    dmc.bytesRemaining = (static_cast<uint16_t>(dmc.sampleLength) << 4) + 1;

    // Request the first sample of the new loop
    bus->requestDmcDma(dmc.currentAddress);
}

//   struct Pulse {
//         // 0x4000 / 0x4004
//         uint8_t volumeOrEnvelopeRate : 4;
//         bool constantVolume : 1;
//         bool envelopeLoopOrLengthCounterHalt : 1;
//         uint8_t duty : 2;

//         // 0x4001 / 0x4005
//         uint8_t sweepUnitShift : 3;
//         bool sweepUnitNegate : 1;
//         uint8_t sweepUnitPeriod : 3;
//         bool sweepUnitEnabled : 1;

//         // 0x4002 / 0x4006
//         uint8_t timerLow;

//         // 0x4003 / 0x4007
//         uint8_t timerHigh : 3;
//         uint8_t lengthCounterLoad : 5;

//         // Internal state
//         uint16_t timerCounter : 11;
//         uint8_t dutyCycleIndex : 3;
//         uint8_t lengthCounter;
//         bool envelopeStartFlag : 1;
//         uint8_t envelope : 4;
//         uint8_t envelopeDividerCounter;
//         uint8_t sweepDividerCounter;
//         bool sweepMutesChannel : 1;
//         bool sweepReloadFlag : 1;

void APU::serialize(Serializer& s) const {
    auto serializePulse = [](Serializer& s, const Pulse& pulse) -> void {
        // 0x4000 / 0x4004
        s.serializeUInt8(pulse.volumeOrEnvelopeRate);
        s.serializeBool(pulse.constantVolume);
        s.serializeBool(pulse.envelopeLoopOrLengthCounterHalt);
        s.serializeUInt8(pulse.duty);

        // 0x4001 / 0x4005
        s.serializeUInt8(pulse.sweepUnitShift);
        s.serializeBool(pulse.sweepUnitNegate);
        s.serializeUInt8(pulse.sweepUnitPeriod);
        s.serializeBool(pulse.sweepUnitEnabled);

        // 0x4002 / 0x4006
        s.serializeUInt8(pulse.timerLow);

        // 0x4003 / 0x4007
        s.serializeUInt8(pulse.timerHigh);
        s.serializeUInt8(pulse.lengthCounterLoad);

        // Internal state
        s.serializeUInt16(pulse.timerCounter);
        s.serializeUInt8(pulse.dutyCycleIndex);
        s.serializeUInt8(pulse.lengthCounter);
        s.serializeBool(pulse.envelopeStartFlag);
        s.serializeUInt8(pulse.envelope);
        s.serializeUInt8(pulse.envelopeDividerCounter);
        s.serializeUInt8(pulse.sweepDividerCounter);
        s.serializeBool(pulse.sweepMutesChannel);
        s.serializeBool(pulse.sweepReloadFlag);
    };
    serializePulse(s, pulses[0]);
    serializePulse(s, pulses[1]);
}

void APU::deserialize(Deserializer& d) {
    auto deserializePulse = [](Deserializer& d, Pulse& pulse) -> void {
        // 0x4000 / 0x4004
        uint8_t volumeOrEnvelopeRateTemp;
        d.deserializeUInt8(volumeOrEnvelopeRateTemp);
        pulse.volumeOrEnvelopeRate = volumeOrEnvelopeRateTemp;

        bool constantVolumeTemp;
        d.deserializeBool(constantVolumeTemp);
        pulse.constantVolume = constantVolumeTemp;

        bool envelopeLoopOrLengthCounterHaltTemp;
        d.deserializeBool(envelopeLoopOrLengthCounterHaltTemp);
        pulse.envelopeLoopOrLengthCounterHalt = envelopeLoopOrLengthCounterHaltTemp;

        uint8_t dutyTemp;
        d.deserializeUInt8(dutyTemp);
        pulse.duty = dutyTemp;

        // 0x4001 / 0x4005
        uint8_t sweepUnitShiftTemp;
        d.deserializeUInt8(sweepUnitShiftTemp);
        pulse.sweepUnitShift = sweepUnitShiftTemp;

        bool sweepUnitNegateTemp;
        d.deserializeBool(sweepUnitNegateTemp);
        pulse.sweepUnitNegate = sweepUnitNegateTemp;

        uint8_t sweepUnitPeriodTemp;
        d.deserializeUInt8(sweepUnitPeriodTemp);
        pulse.sweepUnitPeriod = sweepUnitPeriodTemp;

        bool sweepUnitEnabledTemp;
        d.deserializeBool(sweepUnitEnabledTemp);
        pulse.sweepUnitEnabled = sweepUnitEnabledTemp;

        // 0x4002 / 0x4006
        uint8_t timerLowTemp;
        d.deserializeUInt8(timerLowTemp);
        pulse.timerLow = timerLowTemp;

        // 0x4003 / 0x4007
        uint8_t timerHighTemp;
        d.deserializeUInt8(timerHighTemp);
        pulse.timerHigh = timerHighTemp;

        uint8_t lengthCounterLoadTemp;
        d.deserializeUInt8(lengthCounterLoadTemp);
        pulse.lengthCounterLoad = lengthCounterLoadTemp;

        // Internal state
        uint16_t timerCounterTemp;
        d.deserializeUInt16(timerCounterTemp);
        pulse.timerCounter = timerCounterTemp;

        uint8_t dutyCycleIndexTemp;
        d.deserializeUInt8(dutyCycleIndexTemp);
        pulse.dutyCycleIndex = dutyCycleIndexTemp;

        d.deserializeUInt8(pulse.lengthCounter);

        bool envelopeStartFlagTemp;
        d.deserializeBool(envelopeStartFlagTemp);
        pulse.envelopeStartFlag = envelopeStartFlagTemp;

        uint8_t envelopeTemp;
        d.deserializeUInt8(envelopeTemp);
        pulse.envelope = envelopeTemp;

        d.deserializeUInt8(pulse.envelopeDividerCounter);
        d.deserializeUInt8(pulse.sweepDividerCounter);

        bool sweepMutesChannelTemp;
        d.deserializeBool(sweepMutesChannelTemp);
        pulse.sweepMutesChannel = sweepMutesChannelTemp;

        bool sweepReloadFlagTemp;
        d.deserializeBool(sweepReloadFlagTemp);
        pulse.sweepReloadFlag = sweepReloadFlagTemp;
    };
    deserializePulse(d, pulses[0]);
    deserializePulse(d, pulses[1]);
}