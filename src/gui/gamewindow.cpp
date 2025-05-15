#include "gui/gamewindow.hpp"

#include <QImage>
#include <QPainter>
#include <QPixmap>

GameWindow::GameWindow(QWidget* parent)
    : QWidget(parent) {
    this->setFixedSize(GAME_WIDTH, GAME_HEIGHT);
}

void GameWindow::setCurrentFrame(const QImage& image) {
    currentFrame = image;
    update();
}

void GameWindow::paintEvent(QPaintEvent* /*event*/) {
    if (!currentFrame.isNull()) {
        QPainter painter(this);
        const QPixmap pixmap = QPixmap::fromImage(currentFrame);
        painter.drawPixmap(0, 0, GAME_WIDTH, GAME_HEIGHT, pixmap);
    }
}