#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "core/bus.hpp"

#include <QImage>
#include <QWidget>

class GameWindow : public QWidget {
    Q_OBJECT

public:
    GameWindow(QWidget *parent);
    void setCurrentFrame(const QImage& image);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int GAME_WIDTH = 256 * 3;
    static constexpr int GAME_HEIGHT = 240 * 3;

    static constexpr int FPS = 60;
    QImage currentFrame;
};

#endif // GAMEWINDOW_HPP
