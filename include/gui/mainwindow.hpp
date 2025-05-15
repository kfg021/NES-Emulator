#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "core/bus.hpp"
#include "gui/gamewindow.hpp"
#include "gui/debugwindow.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <string>

#include <QMainWindow>
#include <QWidget>

using ControllerStatus = std::array<std::atomic<uint8_t>, 2>;
struct GameInput {
    const ControllerStatus* controllerStatus;
    std::atomic<bool>* resetFlag;
};

class EmulatorThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent, const std::string& filePath);

#ifdef SHOW_DEBUG_WINDOW
    static constexpr int DEBUG_WIDTH = 300;
#endif

public slots:
    void displayNewFrame(const PPU::Display& display);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    EmulatorThread* emulatorThread;
    GameWindow* gameWindow;

#ifdef SHOW_DEBUG_WINDOW
    DebugWindow* debugWindow;
    void executeCycleAndUpdateDebugWindow();
    void executeInstruction();
    void toggleDebugMode();
    void stepIfInDebugMode();
    bool debugMode;
#endif
    void reset();

    ControllerStatus controllerStatus;
    std::atomic<bool> resetFlag;

    void setControllerData(bool controller, Controller::Button button, bool value);
};

#endif // MAINWINDOW_HPP