#include "core/apu.hpp"

APU::APU() {
    initAPU();
}

void APU::initAPU() {
    pulses[0] = {};
    pulses[1] = {};

    status = 0;
    frameCounter = 0;
    totalCycles = 0;

    frameSequenceMode = false;
    interruptInhibitFlag = false;
    frameInterruptFlag = false;
}

void APU::write(uint16_t addr, uint8_t value) {
    if (PULSE_RANGE.contains(addr)) {
        bool pulseNum = (addr >> 3) & 1;
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
                break;

            case 2: // 0x4002 / 0x4006
                pulse.timerLow = value;
                break;

            default: // 0x4003 / 0x4007
                pulse.timerHigh = value & 0x7;
                uint16_t timerReload = (pulse.timerHigh << 8) | pulse.timerLow;
                pulse.timerCounter = timerReload;

                pulse.lengthCounterLoad = (value >> 3) & 0x1F;
                bool channelEnabled = (status >> pulseNum) & 1;
                if (channelEnabled) {
                    pulse.lengthCounter = LENGTH_COUNTER_TABLE[pulse.lengthCounterLoad];
                }

                pulse.envelopeStartFlag = true;
                pulse.dutyCycleIndex = 0;
                break;
        }
    }
}

uint8_t APU::viewStatus() const {
    uint8_t tempStatus = 0;

    for (int i = 0; i < 2; i++) {
        bool statusBit = (status >> i) & 1;
        if (statusBit && pulses[i].lengthCounter > 0) {
            tempStatus |= (1 << i);
        }
    }

    if (frameInterruptFlag) {
        tempStatus |= (1 << 6);
    }

    // $4015 read	IF-D NT21	DMC interrupt (I), frame interrupt (F), DMC active (D), length counter > 0 (N/T/2/1)
    // TODO: I, D, N, T

    return tempStatus;
}

uint8_t APU::readStatus() {
    frameInterruptFlag = false;
    return viewStatus();
}

void APU::writeStatus(uint8_t value) {
    if (!((value >> 0) & 1)) pulses[0].lengthCounter = 0;
    if (!((value >> 1) & 1)) pulses[1].lengthCounter = 0;

    // TODO: Other channels

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
        for (int i = 0; i < 2; i++) {
            bool statusBit = (status >> i) & 1;
            if (statusBit) {
                Pulse& pulse = pulses[i];
                pulse.timerCounter--;

                if (pulse.timerCounter == 0x7FF) {
                    uint16_t timerReload = (pulse.timerHigh << 8) | pulse.timerLow;
                    pulse.timerCounter = timerReload;

                    pulse.dutyCycleIndex++;
                }
            }
        }
    }

    frameCounter++;
    totalCycles++;
}

void APU::quarterClock() {
    // Clock envelope
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

        // TODO: Triangle envelope
    }

    // TODO: Clock triangle linear counter
    {

    }
}

void APU::halfClock() {
    // Clock length counters
    {
        for (int i = 0; i < 2; i++) {
            bool statusBit = (status >> i) & 1;
            if (statusBit && pulses[i].lengthCounter && !pulses[i].envelopeLoopOrLengthCounterHalt) pulses[i].lengthCounter--;
        }

    }

    // TODO: Clock sweep units
    {

    }
}

bool APU::irqRequested() const {
    return frameInterruptFlag;
}

float APU::getAudioSample() {
    float output = 0.0f;

    // Get pulse 1 output
    {
        Pulse& pulse = pulses[0];
        bool statusBit = (status >> 0) & 1;
        if (statusBit && pulse.lengthCounter > 0 && pulse.timerCounter >= 8) {
            uint8_t dutyCycle = DUTY_CYCLES[pulse.duty];
            bool dutyOutput = (dutyCycle >> pulse.dutyCycleIndex) & 1;

            if (dutyOutput) {
                uint8_t volume = pulse.constantVolume ? pulse.volumeOrEnvelopeRate : pulse.envelope;
                output += volume;
            }
        }
    }

    // TODO: mix other channels

    return 0.1f * (output / 15.0f);
}
