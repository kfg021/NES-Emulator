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
                pulse.lengthCounter = pulse.lengthCounterLoad;
                break;
        }
    }
}

uint8_t APU::viewStatus() {
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
    status = value;
}

void APU::writeFrameCounter(uint8_t value) {
    frameSequenceMode = (value >> 7) & 1;
    interruptInhibitFlag = (value >> 6) & 1;

    frameCounter = 0;
}

void APU::executeCycle() {
    bool quarterClock = false;
    bool halfClock = false;

    if (!frameSequenceMode) {
        // 4 step sequence
        switch (frameCounter % FOUR_STEP_SEQUENCE_LENGTH) {
            case STEP_SEQUENCE[0]:
                quarterClock = true;
                break;
            case STEP_SEQUENCE[1]:
                quarterClock = true;
                halfClock = true;
                break;
            case STEP_SEQUENCE[2]:
                quarterClock = true;
                break;
            case STEP_SEQUENCE[3] - 1:
                frameInterruptFlag = !interruptInhibitFlag;
                break;
            case STEP_SEQUENCE[3]:
                quarterClock = true;
                halfClock = true;
                frameInterruptFlag = !interruptInhibitFlag;
                break;
            case 0:
                if (frameCounter) frameInterruptFlag = !interruptInhibitFlag;
                break;
            default:
                break;
        }
    }
    else {
        // 5 step sequence
        switch (frameCounter % FIVE_STEP_SEQUENCE_LENGTH) {
            case STEP_SEQUENCE[0]:
                quarterClock = true;
                break;
            case STEP_SEQUENCE[1]:
                quarterClock = true;
                halfClock = true;
                break;
            case STEP_SEQUENCE[2]:
                quarterClock = true;
                break;
            case STEP_SEQUENCE[3]:
                // Do nothing
                break;
            case STEP_SEQUENCE[4]:
                quarterClock = true;
                halfClock = true;
                frameInterruptFlag = !interruptInhibitFlag;
                break;
            default:
                break;
        }
    }

    if (quarterClock) {
        // Clock envelope
        {
            for (int i = 0; i < 2; i++) {
                if (pulses[i].envelope) pulses[i].envelope--;

                if (!pulses[i].envelope && pulses[i].envelopeLoopOrLengthCounterHalt) {
                    pulses[i].envelope = 0xF;
                }
            }

        }

        // Clock triangle linear counter
        {

        }
    }

    if (halfClock) {
        // Clock length counters
        {
            for (int i = 0; i < 2; i++) {
                bool statusBit = (status >> i) & 1;
                if (statusBit && pulses[i].lengthCounter) pulses[i].lengthCounter--;
            }

        }

        // Clock sweep units
        {

        }
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

float APU::getAudioSample() {
    float output = 0.0f;

    // Get pulse 1 output
    {
        Pulse& pulse = pulses[0];
        bool statusBit = (status >> 0) & 1;
        if (statusBit && pulse.timerCounter >= 8) {
            uint8_t dutyCycle = DUTY_CYCLES[pulse.duty];
            bool dutyOutput = (dutyCycle >> pulse.dutyCycleIndex) & 1;

            if (dutyOutput) {
                uint8_t volume = pulse.constantVolume ? pulse.volumeOrEnvelopeRate : pulse.envelope;
                output += volume;
            }
        }
    }

    return output / 7.5f - 1; // Range [-1, 1]
}
