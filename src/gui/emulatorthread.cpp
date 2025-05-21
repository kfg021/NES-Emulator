#include "gui/emulatorthread.hpp"

#include <cstdint>

#include <QElapsedTimer>

EmulatorThread::EmulatorThread(QObject* parent, const std::string& filePath, const KeyboardInput& keyInput, ThreadSafeQueue<float>* audioSamples) :
    QThread(parent),
    keyInput(keyInput),
    audioSamples(audioSamples) {
    isRunning.store(false, std::memory_order_relaxed);

    Cartridge::Status status = bus.loadROM(filePath);
    if (status.code != Cartridge::Code::SUCCESS) {
        qFatal("%s", status.message.c_str());
    }

    scaledAudioClock = 0;
}

EmulatorThread::~EmulatorThread() {
    isRunning.store(false, std::memory_order_release);
    wait();
}

void EmulatorThread::requestStop() {
    isRunning.store(false, std::memory_order_release);
}

void EmulatorThread::run() {
    isRunning.store(true, std::memory_order_relaxed);

    int desiredNextFrameUs = TARGET_FRAME_US;
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    while (isRunning.load(std::memory_order_acquire)) {
        // Check reset
        bool resetRequested = keyInput.resetFlag->load(std::memory_order_relaxed);
        if (resetRequested) {
            bus.initDevices();

            audioSamples->erase();
            scaledAudioClock = 0;

            std::queue<uint16_t> empty;
            std::swap(recentPCs, empty);

            keyInput.resetFlag->store(false, std::memory_order_relaxed);

            desiredNextFrameUs = (elapsedTimer.nsecsElapsed() / 1000) + TARGET_FRAME_US;
        }

        // Handle controller input
        uint8_t controller1Value = (*keyInput.controllerStatus)[0].load(std::memory_order_relaxed);
        uint8_t controller2Value = (*keyInput.controllerStatus)[1].load(std::memory_order_relaxed);
        bus.setController(0, controller1Value);
        bus.setController(1, controller2Value);

        runCycles();

        if (!isRunning.load(std::memory_order_relaxed)) break;

        if (bus.ppu->frameReadyFlag) {
            const PPU::Display& display = *(bus.ppu->finishedDisplay);
            QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format::Format_ARGB32);
            emit frameReadySignal(image.copy());
            bus.ppu->frameReadyFlag = false;
        }

        if (keyInput.debugWindowEnabled->load(std::memory_order_relaxed)) {
            uint8_t backgroundPallete = keyInput.backgroundPallete->load(std::memory_order_relaxed) & 3;
            uint8_t spritePallete = keyInput.spritePallete->load(std::memory_order_relaxed) & 3;

            DebugWindowState state = {
                bus.cpu->getPC(),
                bus.cpu->getA(),
                bus.cpu->getX(),
                bus.cpu->getY(),
                bus.cpu->getSP(),
                bus.cpu->getSR(),
                backgroundPallete,
                spritePallete,
                bus.ppu->getPalleteRamColors(),
                bus.ppu->getPatternTable(true, backgroundPallete),
                bus.ppu->getPatternTable(false, spritePallete),
                getInsts()
            };

            emit debugFrameReadySignal(state);
        }
        else {
            if (!recentPCs.empty()) {
                std::queue<uint16_t> empty;
                std::swap(recentPCs, empty);
            }
        }

        int currentTimeUs = elapsedTimer.nsecsElapsed() / 1000;
        int sleepTimeUs = desiredNextFrameUs - currentTimeUs;

        // Small sleep times are generally unreliable
        static constexpr int MIN_SLEEP_TIME_US = 1000;
        if (sleepTimeUs > MIN_SLEEP_TIME_US) {
            QThread::usleep(sleepTimeUs);
        }
        else if (sleepTimeUs > 0) {
            QThread::yieldCurrentThread();
        }
        else {
            // We missed the deadline for this frame, so reset the frame deadline.
            desiredNextFrameUs = currentTimeUs;

            QThread::yieldCurrentThread();
        }

        desiredNextFrameUs += TARGET_FRAME_US;
    }
}

void EmulatorThread::runCycles() {
    int cycles = 0;

    auto executeCycle = [&](bool debugEnabled, bool audioEnabled) -> bool {
        uint16_t currentPC = bus.cpu->getPC();

        bus.executeCycle();
        cycles++;

        uint16_t nextPC = bus.cpu->getPC();

        if (audioEnabled) {
            while (scaledAudioClock >= INSTRUCTIONS_PER_SECOND) {
                float sample = bus.apu->getAudioSample();
                audioSamples->push(sample);
                scaledAudioClock -= INSTRUCTIONS_PER_SECOND;
            }
            scaledAudioClock += AUDIO_SAMPLE_RATE;
        }
        else {
            scaledAudioClock = 0;
        }

        if (nextPC != currentPC) {
            if (debugEnabled) {
                if (recentPCs.size() == DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW) {
                    recentPCs.pop();
                }
                recentPCs.push(currentPC);
            }
            return true;
        }
        return false;
    };

    auto loopCondition = [&]() -> bool {
        static constexpr int UPPER_LIMIT_CYCLES = EXPECTED_CPU_CYCLES_PER_FRAME * 2;
        return isRunning.load(std::memory_order_relaxed) && !bus.ppu->frameReadyFlag && cycles < UPPER_LIMIT_CYCLES;
    };

    bool stepModeEnabled = keyInput.stepModeEnabled->load(std::memory_order_relaxed) & 1;
    if (!stepModeEnabled) {
        bool debugEnabled = keyInput.debugWindowEnabled->load(std::memory_order_relaxed);
        bool audioEnabled = !(keyInput.globalMuteFlag->load(std::memory_order_relaxed) & 1);
        while (loopCondition()) {
            executeCycle(debugEnabled, audioEnabled);
        }
    }
    else if (stepModeEnabled && keyInput.stepRequested->load(std::memory_order_acquire)) {
        keyInput.stepRequested->store(false, std::memory_order_release);

        while (loopCondition()) {
            bool isNewInstruction = executeCycle(true, false);
            if (isNewInstruction) break;
        }
    }
}

std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> EmulatorThread::getInsts() const {
    std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> insts;
    std::queue<uint16_t> recentPCsCopy = recentPCs;

    auto toString = [&](uint16_t addr) {
        std::string text = "$" + toHexString16(addr) + ": " + bus.cpu->toString(addr);
        return QString(text.c_str());
    };

    // Last x PCs
    int start = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW - static_cast<int>(recentPCsCopy.size());
    for (int i = start; i < DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW; i++) {
        insts[i] = toString(recentPCsCopy.front());
        recentPCsCopy.pop();
    }

    // Current PC and upcoming x PCs
    uint16_t lastPC = bus.cpu->getPC();
    for (int i = DebugWindowState::NUM_INSTS_ABOVE_AND_BELOW; i < DebugWindowState::NUM_INSTS_TOTAL; i++) {
        insts[i] = toString(lastPC);
        const CPU::Opcode& op = bus.cpu->getOpcode(lastPC);
        lastPC += op.addressingMode.instructionSize;
    }

    return insts;
}