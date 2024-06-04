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

    static const int GAME_WIDTH = 256 * 3;
    static const int GAME_HEIGHT = 240 * 3;
    static const int DEBUG_WIDTH = 300;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    GameWindow* gameWindow;
    DebugWindow* debugWindow;

    std::shared_ptr<CPU> cpu;
};

#endif // MAINWINDOW_HPP