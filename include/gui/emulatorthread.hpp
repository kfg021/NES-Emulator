#ifndef EMULATORTHREAD_HPP
#define EMULATORTHREAD_HPP

#include "core/bus.hpp"
#include "gui/debugwindow.hpp"
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
    EmulatorThread(QObject* parent, const std::string& filePath, const KeyboardInput& keyInput, ThreadSafeQueue<float>* queue);
    ~EmulatorThread() override;
    void run() override;

    void requestStop();

    static constexpr int AUDIO_SAMPLE_RATE = 44100;

signals:
    void frameReadySignal(const QImage& display);
    void debugFrameReadySignal(const DebugWindowState& state);

private:
    static constexpr int FPS = 60;
    static constexpr int INSTRUCTIONS_PER_SECOND = 1773448;
    static constexpr int TARGET_FRAME_NS = static_cast<int>(1e9) / FPS;
    static constexpr int EXPECTED_CPU_CYCLES = INSTRUCTIONS_PER_SECOND / FPS;

    Bus bus;
    std::atomic<bool> isRunning;

    // EmulatorThread is not responsible for managing this memory
    KeyboardInput keyInput;

    void runCycles();
    std::queue<uint16_t> recentPCs;
    std::array<QString, DebugWindowState::NUM_INSTS_TOTAL> getInsts() const;

    ThreadSafeQueue<float>* queue;

    int scaledAudioClock;
};

#endif // EMULATORTHREAD_HPP