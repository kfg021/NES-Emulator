#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "../core/cpu.hpp"

#include "gamewindow.hpp"
#include "debugwindow.hpp"

#include <string>
#include <memory>

#include <QMainWindow>
#include <QWidget>


class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    MainWindow(QWidget* parent, const std::string& filePath);

    static constexpr int GAME_WIDTH = 256 * 3;
    static constexpr int GAME_HEIGHT = 240 * 3;

#ifdef SHOW_DEBUG_WINDOW
    static constexpr int DEBUG_WIDTH = 300;
#endif

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    
private:
    GameWindow* gameWindow;

#ifdef SHOW_DEBUG_WINDOW
    DebugWindow* debugWindow;
#endif

    QTimer* updateTimer;
    QElapsedTimer* elapsedTimer;

    static constexpr int FPS = 60;
    static constexpr int IPS = 5369318;

    void executeCycle();
    void executeInstruction();

#ifdef SHOW_DEBUG_WINDOW
    void toggleDebugMode();
    void stepIfInDebugMode();
    bool debugMode;
#endif

    int64_t numSteps;
    int64_t numFrames;
    void tick();

    void reset();

    std::shared_ptr<Bus> bus;
};

#endif // MAINWINDOW_HPP