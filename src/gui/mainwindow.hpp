#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "../core/cpu.hpp"

#include "gamewindow.hpp"
#include "debugwindow.hpp"

#include <memory>

#include <QMainWindow>
#include <QWidget>


class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

    static constexpr int GAME_WIDTH = 256 * 3;
    static constexpr int GAME_HEIGHT = 240 * 3;
    static constexpr int DEBUG_WIDTH = 300;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    
private:
    GameWindow* gameWindow;
    DebugWindow* debugWindow;

    QTimer* updateTimer;
    QElapsedTimer* elapsedTimer;

    static constexpr int FPS = 60;
    static constexpr int IPS = 5369318;

    void executeCycle();
    void executeInstruction();

    void toggleDebugMode();
    void stepIfInDebugMode();

    bool debugMode;

    int64_t numSteps;
    int64_t numFrames;
    void tick();

    std::shared_ptr<Bus> bus;
};

#endif // MAINWINDOW_HPP