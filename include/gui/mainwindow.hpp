#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "core/cpu.hpp"

#include "gui/gamewindow.hpp"
#include "gui/debugwindow.hpp"

#include <memory>
#include <string>

#include <QMainWindow>
#include <QWidget>


class MainWindow : public QMainWindow {
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

private slots:
    void tick();

private:
    static constexpr int TICKS_PER_SECOND = 60;
    static constexpr int INSTRUCTIONS_PER_SECOND = 1773448;

    GameWindow* gameWindow;

    QTimer* updateTimer;
    QTimer* renderTimer;
    QElapsedTimer* elapsedTimer;

#ifdef SHOW_DEBUG_WINDOW
    DebugWindow* debugWindow;
    void executeCycleAndUpdateDebugWindow();
    void executeInstruction();
    void toggleDebugMode();
    void stepIfInDebugMode();
    bool debugMode;
#endif

    int64_t numSteps;

    void reset();

    std::shared_ptr<Bus> bus;
};

#endif // MAINWINDOW_HPP