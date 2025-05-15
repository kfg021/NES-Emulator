#ifndef EMULATORTHREAD_HPP
#define EMULATORTHREAD_HPP

#include "core/bus.hpp"
#include "core/ppu.hpp"
#include "gui/mainwindow.hpp"

#include <atomic>
#include <memory>

#include <QObject>
#include <QThread>

class EmulatorThread : public QThread {
    Q_OBJECT
public:
    EmulatorThread(QObject* parent, const std::string& filePath, const GameInput& gameInput);
    ~EmulatorThread() override;
    void run() override;

signals:
    void frameReadySignal(const PPU::Display& display);

private:
    constexpr static int FPS = 60;
    static constexpr int INSTRUCTIONS_PER_SECOND = 1773448;
    Bus bus;
    std::atomic<bool> isRunning;

    // EmulatorThread is not responsible for managing this memory
    GameInput gameInput;
};

#endif // EMULATORTHREAD_HPP