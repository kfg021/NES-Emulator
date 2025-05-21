#ifndef EMULATORTHREAD_HPP
#define EMULATORTHREAD_HPP

#include "core/bus.hpp"
#include "gui/guitypes.hpp"
#include "util/threadsafequeue.hpp"

#include <array>
#include <queue>

#include <QImage>
#include <QObject>
#include <QString>
#include <QThread>

class EmulatorThread : public QThread {
    Q_OBJECT
public:
    EmulatorThread(QObject* parent, const std::string& filePath, const KeyboardInput& keyInput, ThreadSafeQueue<float>* audioSamples);
    ~EmulatorThread() override;
    void run() override;

    void requestStop();

    static constexpr int AUDIO_SAMPLE_RATE = 44100;

signals:
    void frameReadySignal(const QImage& display);
    void debugFrameReadySignal(const DebugWindowState& state);

private:
    static constexpr int FPS = 60;
    static constexpr int TARGET_FRAME_US = 1000000 / FPS;

    // 262 scanlines, 341 cycles per scanline, 3 PPU cycles per CPU cycle
    static constexpr int EXPECTED_CPU_CYCLES_PER_FRAME = (262 * 341) / 3;
    static constexpr int INSTRUCTIONS_PER_SECOND = EXPECTED_CPU_CYCLES_PER_FRAME * FPS;

    Bus bus;
    std::atomic<bool> isRunning;

    // EmulatorThread is not responsible for managing this memory
    KeyboardInput keyInput;

    void runCycles();
    std::queue<uint16_t> recentPCs;
    std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> getInsts() const;

    ThreadSafeQueue<float>* audioSamples;

    int scaledAudioClock;
};

#endif // EMULATORTHREAD_HPP