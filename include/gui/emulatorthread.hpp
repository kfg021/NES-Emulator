#ifndef EMULATORTHREAD_HPP
#define EMULATORTHREAD_HPP

#include "core/bus.hpp"
#include "gui/guitypes.hpp"

#include <array>
#include <queue>

#include <QImage>
#include <QObject>
#include <QString>
#include <QThread>

class EmulatorThread : public QThread {
    Q_OBJECT
public:
    EmulatorThread(QObject* parent, const std::string& filePath, const KeyboardInput& keyInput);
    ~EmulatorThread() override;
    void run() override;

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
};

#endif // EMULATORTHREAD_HPP