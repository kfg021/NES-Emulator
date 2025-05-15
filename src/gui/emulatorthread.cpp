#include "gui/emulatorthread.hpp"

#include <cstdint>

#include <QElapsedTimer>

EmulatorThread::EmulatorThread(QObject* parent, const std::string& filePath, const GameInput& gameInput) :
    QThread(parent),
    gameInput(gameInput) {
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
    static constexpr int64_t NANOSECONDS = static_cast<int64_t>(1e9);
    static constexpr int64_t TARGET_FRAME_NS = NANOSECONDS / FPS;
    static constexpr int EXPECTED_CPU_CYCLES = INSTRUCTIONS_PER_SECOND / FPS;

    isRunning.store(true, std::memory_order_relaxed);

    int64_t desiredNextFrameNs = TARGET_FRAME_NS; 
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    while (isRunning.load(std::memory_order_acquire)) {
        // Check reset
        bool resetRequested = gameInput.resetFlag->load(std::memory_order_relaxed);
        if (resetRequested) {
            // TODO: This is technically this is not a reset. It is more like a "power on"
            bus.initDevices();
            gameInput.resetFlag->store(false, std::memory_order_relaxed);
        }

        // Handle controller input
        uint8_t controller1Value = (*gameInput.controllerStatus)[0].load(std::memory_order_relaxed);
        uint8_t controller2Value = (*gameInput.controllerStatus)[1].load(std::memory_order_relaxed);
        bus.setController(0, controller1Value);
        bus.setController(1, controller2Value);

        static constexpr int UPPER_LIMIT_CYCLES = EXPECTED_CPU_CYCLES * 2;
        int cycles = 0;
        while (isRunning.load(std::memory_order_relaxed) && !bus.ppu->frameReadyFlag && cycles < UPPER_LIMIT_CYCLES) {
            bus.executeCycle();
            cycles++;
        }

        if (!isRunning.load(std::memory_order_relaxed)) break;
        
        if (bus.ppu->frameReadyFlag) {
            const PPU::Display& display = *(bus.ppu->finishedDisplay);
            QImage image(reinterpret_cast<const uint8_t*>(&display), 256, 240, QImage::Format::Format_ARGB32);
            emit frameReadySignal(image.copy());
            bus.ppu->frameReadyFlag = false;
        }

        int64_t sleepTimeUs = (desiredNextFrameNs - elapsedTimer.nsecsElapsed()) / 1000;
        if (sleepTimeUs > 0) {
            QThread::usleep(sleepTimeUs);
        }

        desiredNextFrameNs += TARGET_FRAME_NS;
    }
}