#include "gui/emulatorthread.hpp"

#include <cstdint>

#include <QElapsedTimer>

EmulatorThread::EmulatorThread(QObject* parent, const std::string& filePath, const KeyboardInput& keyInput) :
    QThread(parent),
    keyInput(keyInput) {
    isRunning.store(false, std::memory_order_relaxed);

    Cartridge::Status status = bus.loadROM(filePath);
    if (status.code != Cartridge::Code::SUCCESS) {
        qFatal("%s", status.message.c_str());
    }
}

EmulatorThread::~EmulatorThread() {
    isRunning.store(false, std::memory_order_release);
    wait();
}

void EmulatorThread::run() {
    isRunning.store(true, std::memory_order_relaxed);

    int64_t desiredNextFrameNs = TARGET_FRAME_NS;
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    while (isRunning.load(std::memory_order_acquire)) {
        // Check reset
        bool resetRequested = keyInput.resetFlag->load(std::memory_order_relaxed);
        if (resetRequested) {
            // TODO: This is technically this is not a reset. It is more like a "power on"
            bus.initDevices();
            keyInput.resetFlag->store(false, std::memory_order_relaxed);
        }

        // Handle controller input
        uint8_t controller1Value = (*keyInput.controllerStatus)[0].load(std::memory_order_relaxed);
        uint8_t controller2Value = (*keyInput.controllerStatus)[1].load(std::memory_order_relaxed);
        bus.setController(0, controller1Value);
        bus.setController(1, controller2Value);

        // Check debug window status
        bool debugWindowOpen = keyInput.debugWindowEnabled->load(std::memory_order_relaxed);

        std::queue<uint16_t> recentPCs = runCycles();

        if (!isRunning.load(std::memory_order_relaxed)) break;

        if (bus.ppu->frameReadyFlag) {
            const PPU::Display& display = *(bus.ppu->finishedDisplay);
            QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format::Format_ARGB32);
            emit frameReadySignal(image.copy());
            bus.ppu->frameReadyFlag = false;
        }

        if (debugWindowOpen) {
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
                recentPCs
            };

            emit debugFrameReadySignal(state);
        }

        int64_t sleepTimeUs = (desiredNextFrameNs - elapsedTimer.nsecsElapsed()) / 1000;
        if (sleepTimeUs > 0) {
            QThread::usleep(sleepTimeUs);
        }

        desiredNextFrameNs += TARGET_FRAME_NS;
    }
}

std::queue<uint16_t> EmulatorThread::runCycles() {
    static constexpr int UPPER_LIMIT_CYCLES = EXPECTED_CPU_CYCLES * 2;
    int cycles = 0;
    std::queue<uint16_t> recentPCs;

    auto loopCondition = [&]() -> bool {
        return isRunning.load(std::memory_order_relaxed) && !bus.ppu->frameReadyFlag && cycles < UPPER_LIMIT_CYCLES;
    };

    while (loopCondition()) {
        while (bus.cpu->getRemainingCycles() && loopCondition()) {
            bus.executeCycle();
            cycles++;
        }

        if (!loopCondition()) break;

        // Update the list of recent PCs so we can display them in the debug window
        uint16_t currentPC = bus.cpu->getPC();

        bus.executeCycle();
        cycles++;

        if (recentPCs.size() == DebugWindow::NUM_INSTS) {
            recentPCs.pop();
        }
        recentPCs.push(currentPC);
    }

    return recentPCs;
}