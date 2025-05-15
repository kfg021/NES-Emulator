#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "core/bus.hpp"
#include "core/ppu.hpp"

#include <memory>
#include <mutex>

#include <QWidget>

class GameWindow : public QWidget {
    Q_OBJECT

public:
    GameWindow(QWidget *parent);
    void setCurrentFrame(const PPU::Display& display);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int GAME_WIDTH = 256 * 3;
    static constexpr int GAME_HEIGHT = 240 * 3;

    static constexpr int FPS = 60;
    std::mutex mtx;
    QImage currentFrame;
};

#endif // GAMEWINDOW_HPP
